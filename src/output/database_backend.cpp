// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file database_backend.cpp

#include "output/database_backend.h"

#include <algorithm>
#include <atomic>
#include <cctype>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <deque>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <unordered_map>
#if defined(_WIN32)
#include <io.h>
#else
#include <unistd.h>
#endif

#include "logging/logger.h"
#include "output/database/db_schema_validator.h"
#include "output/database/db_url_parser.h"
#include "output/database/drivers/idatabase_driver.h"
#include "output/database/type_adapter.h"
#include "utils/env_utils.h"

namespace datagen::output {

namespace {

using RowWithIndex = std::pair<std::uint64_t, engine::Row>;

constexpr std::size_t kProgressClearWidth   = 160;
constexpr const char* kAnsiClearLine        = "\x1b[2K";
constexpr const char* kAnsiMoveUp           = "\x1b[1A";
constexpr double      kMinElapsedSeconds    = 0.001;
constexpr double      kProgressSleepSeconds = 0.2;

bool stdout_is_tty() {
#if defined(_WIN32)
    return _isatty(_fileno(stdout)) != 0;
#else
    return isatty(fileno(stdout)) != 0;
#endif
}

std::string bool_to_text(const bool value) {
    return value ? "true" : "false";
}

std::string escape_log_text(const std::string& text) {
    std::string escaped;
    escaped.reserve(text.size());
    for (const char ch : text) {
        switch (ch) {
        case '\\': escaped += "\\\\"; break;
        case '\n': escaped += "\\n"; break;
        case '\r': escaped += "\\r"; break;
        case '\t': escaped += "\\t"; break;
        default  : escaped.push_back(ch); break;
        }
    }
    return escaped;
}

std::string json_value_to_log_text(const config::Json& value) {
    if (value.is_string()) { return escape_log_text(value.get<std::string>()); }
    if (value.is_boolean()) { return bool_to_text(value.get<bool>()); }
    if (value.is_number_integer() || value.is_number_unsigned() || value.is_number_float()) { return value.dump(); }
    if (value.is_null()) { return "null"; }
    return value.dump();
}

bool is_scalar_json_value(const config::Json& value) {
    return value.is_string() || value.is_boolean() || value.is_number() || value.is_null();
}

void append_field_log_entries(
    const config::Json&       value,
    const std::string&        prefix,
    std::vector<std::string>* entries
) {
    if (!entries || prefix.empty()) { return; }

    if (value.is_object()) {
        for (auto it = value.begin(); it != value.end(); ++it) {
            append_field_log_entries(it.value(), prefix + "." + it.key(), entries);
        }
        return;
    }

    if (value.is_array()) {
        bool all_scalar = true;
        for (const auto& item : value) {
            if (!is_scalar_json_value(item)) {
                all_scalar = false;
                break;
            }
        }

        if (all_scalar) {
            std::ostringstream oss;
            bool               first = true;
            for (const auto& item : value) {
                if (!first) { oss << "|"; }
                first = false;
                oss << json_value_to_log_text(item);
            }
            entries->push_back(prefix + "=" + oss.str());
        } else {
            entries->push_back(prefix + "=" + value.dump());
        }
        return;
    }

    entries->push_back(prefix + "=" + json_value_to_log_text(value));
}

std::string build_field_log_line(const config::FieldSpec& field) {
    std::ostringstream       oss;
    std::vector<std::string> entries;
    oss << "Field name=" << field.name << ", generator=" << field.generator;

    if (field.raw.is_object()) {
        for (auto it = field.raw.begin(); it != field.raw.end(); ++it) {
            if (it.key() == "name" || it.key() == "generator") { continue; }
            append_field_log_entries(it.value(), it.key(), &entries);
        }
    }

    for (const auto& entry : entries) { oss << ", " << entry; }
    return oss.str();
}

config::InsertMode choose_insert_mode(const config::GenerationConfig& cfg) {
    return cfg.output.database.insert_mode;
}

template <typename T>
class BoundedQueue {
public:
    explicit BoundedQueue(std::size_t capacity) : capacity_(std::max<std::size_t>(1, capacity)) {}

    bool push(T value, const std::atomic<bool>* stop_flag) {
        std::unique_lock lock(mutex_);
        can_push_.wait(lock, [&] { return closed_ || queue_.size() < capacity_ || (stop_flag && stop_flag->load()); });
        if (closed_ || (stop_flag && stop_flag->load())) { return false; }

        queue_.push_back(std::move(value));
        can_pop_.notify_one();
        return true;
    }

    bool pop(T* out, const std::atomic<bool>* stop_flag) {
        std::unique_lock lock(mutex_);
        can_pop_.wait(lock, [&] { return closed_ || !queue_.empty() || (stop_flag && stop_flag->load()); });

        if (queue_.empty()) { return false; }

        *out = std::move(queue_.front());
        queue_.pop_front();
        can_push_.notify_one();
        return true;
    }

    void close() {
        std::lock_guard lock(mutex_);
        closed_ = true;
        can_push_.notify_all();
        can_pop_.notify_all();
    }

private:
    std::size_t             capacity_;
    std::deque<T>           queue_;
    bool                    closed_ = false;
    std::mutex              mutex_;
    std::condition_variable can_push_;
    std::condition_variable can_pop_;
};

std::string join_columns(const std::vector<std::string>& columns) {
    std::ostringstream oss;
    for (std::string::size_type i = 0; i < columns.size(); ++i) {
        if (i > 0) { oss << ", "; }
        oss << columns[i];
    }
    return oss.str();
}

std::string quote_identifier_segment(const database::DbType db_type, const std::string& value) {
    const char  quote = db_type == database::DbType::Mysql ? '`' : '"';
    std::string escaped;
    escaped.reserve(value.size() + 2);
    for (const char ch : value) {
        escaped.push_back(ch);
        if (ch == quote) { escaped.push_back(ch); }
    }
    return std::string(1, quote) + escaped + std::string(1, quote);
}

std::string quote_identifier_path(const database::DbType db_type, const std::string& value) {
    std::ostringstream oss;
    std::size_t        begin = 0;
    bool               first = true;
    while (begin <= value.size()) {
        const std::size_t end  = value.find('.', begin);
        const std::string part = end == std::string::npos ? value.substr(begin) : value.substr(begin, end - begin);
        if (!first) { oss << "."; }
        oss << quote_identifier_segment(db_type, part);
        if (end == std::string::npos) { break; }
        begin = end + 1;
        first = false;
    }
    return oss.str();
}

std::string sql_escape_single_quote(const std::string& value) {
    std::string escaped;
    escaped.reserve(value.size() + 8);
    for (const char ch : value) {
        escaped.push_back(ch);
        if (ch == '\'') { escaped.push_back('\''); }
    }
    return escaped;
}

std::string escape_for_load_file(const std::string& value) {
    std::string escaped;
    escaped.reserve(value.size());
    for (const char ch : value) {
        switch (ch) {
        case '\\': escaped += "\\\\"; break;
        case '\t': escaped += "\\t"; break;
        case '\n': escaped += "\\n"; break;
        case '\r': escaped += "\\r"; break;
        default  : escaped.push_back(ch); break;
        }
    }
    return escaped;
}

std::string render_row_for_load(const std::vector<std::string>& sql_literals) {
    std::ostringstream oss;
    for (std::string::size_type i = 0; i < sql_literals.size(); ++i) {
        if (i > 0) { oss << '\t'; }
        if (sql_literals[i] == "NULL") {
            oss << "\\N";
        } else if (sql_literals[i].size() >= 2 && sql_literals[i].front() == '\'' && sql_literals[i].back() == '\'') {
            const std::string value = sql_literals[i].substr(1, sql_literals[i].size() - 2);
            oss << escape_for_load_file(value);
        } else {
            oss << escape_for_load_file(sql_literals[i]);
        }
    }
    oss << "\n";
    return oss.str();
}

std::string build_insert_sql(
    const database::DbType                       db_type,
    const std::string&                           table_name,
    const std::vector<std::string>&              columns,
    const std::vector<std::vector<std::string>>& rows
) {
    std::ostringstream oss;
    if (rows.empty()) { return ""; }

    if (db_type == database::DbType::Oracle) {
        oss << "INSERT ALL ";
        for (const auto& row_values : rows) {
            oss << "INTO " << table_name << " (" << join_columns(columns) << ") VALUES (";
            for (std::string::size_type col_index = 0; col_index < row_values.size(); ++col_index) {
                if (col_index > 0) { oss << ", "; }
                oss << row_values[col_index];
            }
            oss << ") ";
        }
        oss << "SELECT 1 FROM DUAL";
        return oss.str();
    }

    oss << "INSERT INTO " << table_name << " (" << join_columns(columns) << ") VALUES ";
    for (std::string::size_type row_index = 0; row_index < rows.size(); ++row_index) {
        if (row_index > 0) { oss << ", "; }
        oss << "(";
        for (std::string::size_type col_index = 0; col_index < rows[row_index].size(); ++col_index) {
            if (col_index > 0) { oss << ", "; }
            oss << rows[row_index][col_index];
        }
        oss << ")";
    }

    return oss.str();
}

std::string build_sqlite_postgresql_upsert_sql(
    const std::string&                           table_name,
    const std::vector<std::string>&              columns,
    const std::vector<std::string>&              key_columns,
    const std::vector<std::vector<std::string>>& rows
) {
    std::ostringstream oss;
    oss << build_insert_sql(database::DbType::Postgresql, table_name, columns, rows)
        << " ON CONFLICT (" << join_columns(key_columns) << ") DO UPDATE SET ";
    for (std::string::size_type i = 0; i < columns.size(); ++i) {
        if (i > 0) { oss << ", "; }
        oss << columns[i] << " = excluded." << columns[i];
    }
    return oss.str();
}

std::string build_mysql_upsert_sql(
    const std::string&                           table_name,
    const std::vector<std::string>&              columns,
    const std::vector<std::vector<std::string>>& rows
) {
    std::ostringstream oss;
    oss << build_insert_sql(database::DbType::Mysql, table_name, columns, rows) << " ON DUPLICATE KEY UPDATE ";
    for (std::string::size_type i = 0; i < columns.size(); ++i) {
        if (i > 0) { oss << ", "; }
        oss << columns[i] << " = VALUES(" << columns[i] << ")";
    }
    return oss.str();
}

std::string build_oracle_upsert_sql(
    const std::string&                           table_name,
    const std::vector<std::string>&              columns,
    const std::vector<std::size_t>&              key_indexes,
    const std::vector<std::vector<std::string>>& rows
) {
    if (rows.empty()) { return ""; }

    std::ostringstream oss;
    oss << "MERGE INTO " << table_name << " target USING (";
    for (std::string::size_type row_index = 0; row_index < rows.size(); ++row_index) {
        if (row_index > 0) { oss << " UNION ALL "; }
        oss << "SELECT ";
        for (std::string::size_type col_index = 0; col_index < columns.size(); ++col_index) {
            if (col_index > 0) { oss << ", "; }
            oss << rows[row_index][col_index] << " AS c" << col_index;
        }
        oss << " FROM DUAL";
    }
    oss << ") source ON (";
    for (std::string::size_type key_pos = 0; key_pos < key_indexes.size(); ++key_pos) {
        if (key_pos > 0) { oss << " AND "; }
        oss << "target." << columns[key_indexes[key_pos]] << " = source.c" << key_indexes[key_pos];
    }
    oss << ") WHEN MATCHED THEN UPDATE SET ";
    for (std::string::size_type col_index = 0; col_index < columns.size(); ++col_index) {
        if (col_index > 0) { oss << ", "; }
        oss << "target." << columns[col_index] << " = source.c" << col_index;
    }
    oss << " WHEN NOT MATCHED THEN INSERT (" << join_columns(columns) << ") VALUES (";
    for (std::string::size_type col_index = 0; col_index < columns.size(); ++col_index) {
        if (col_index > 0) { oss << ", "; }
        oss << "source.c" << col_index;
    }
    oss << ")";
    return oss.str();
}

std::optional<std::vector<std::size_t>> determine_upsert_key_indexes(
    const database::TableMetadata&      metadata,
    const std::vector<std::string>&     columns,
    const std::vector<const database::ColumnMetadata*>& mapped_columns
) {
    for (std::string::size_type i = 0; i < mapped_columns.size(); ++i) {
        if (mapped_columns[i]->auto_increment) { return std::vector<std::size_t>{i}; }
    }

    std::unordered_map<std::string, std::size_t> column_to_index;
    for (std::string::size_type i = 0; i < columns.size(); ++i) { column_to_index[columns[i]] = i; }

    std::optional<std::vector<std::size_t>> best_key;
    for (const auto& index : metadata.indexes) {
        if (!index.unique || index.columns.empty()) { continue; }

        std::vector<std::size_t> indexes;
        bool                     all_present = true;
        for (const auto& column_name : index.columns) {
            if (!column_to_index.contains(column_name)) {
                all_present = false;
                break;
            }
            indexes.push_back(column_to_index.at(column_name));
        }

        if (!all_present) { continue; }
        if (!best_key.has_value() || indexes.size() < best_key->size()) { best_key = std::move(indexes); }
    }

    return best_key;
}

std::string build_upsert_sql(
    const database::DbType                       db_type,
    const std::string&                           table_name,
    const std::vector<std::string>&              columns,
    const std::vector<std::size_t>&              key_indexes,
    const std::vector<std::vector<std::string>>& rows
) {
    std::vector<std::string> key_columns;
    key_columns.reserve(key_indexes.size());
    for (const std::size_t key_index : key_indexes) { key_columns.push_back(columns[key_index]); }

    switch (db_type) {
    case database::DbType::Mysql:
        return build_mysql_upsert_sql(table_name, columns, rows);
    case database::DbType::Postgresql:
    case database::DbType::Sqlite:
        return build_sqlite_postgresql_upsert_sql(table_name, columns, key_columns, rows);
    case database::DbType::Oracle:
        return build_oracle_upsert_sql(table_name, columns, key_indexes, rows);
    default:
        return "";
    }
}

std::string build_load_sql(
    const database::DbType          db_type,
    const std::string&              table_name,
    const std::vector<std::string>& columns,
    const std::string&              path
) {
    std::ostringstream oss;
    if (db_type == database::DbType::Mysql) {
        oss << "LOAD DATA LOCAL INFILE '" << sql_escape_single_quote(path) << "' INTO TABLE " << table_name
            << R"( FIELDS TERMINATED BY '\t' ESCAPED BY '\\' LINES TERMINATED BY '\n' ()" << join_columns(columns)
            << ")";
        return oss.str();
    }
    if (db_type == database::DbType::Postgresql) {
        oss << "COPY " << table_name << " (" << join_columns(columns) << ") FROM '" << sql_escape_single_quote(path)
            << R"(' WITH (FORMAT text, DELIMITER E'\t', NULL '\\N'))";
        return oss.str();
    }
    return "";
}

bool begin_transaction(const database::DbType db_type, database::IDatabaseDriver* driver, std::string* error_message) {
    if (!driver) {
        if (error_message) { *error_message = "driver pointer is null"; }
        return false;
    }
    if (db_type == database::DbType::Oracle) { return true; }
    if (db_type == database::DbType::Sqlite) { return driver->execute("BEGIN TRANSACTION", error_message); }
    return driver->execute("START TRANSACTION", error_message);
}

bool commit_transaction(database::IDatabaseDriver* driver, std::string* error_message) {
    if (!driver) {
        if (error_message) { *error_message = "driver pointer is null"; }
        return false;
    }
    return driver->execute("COMMIT", error_message);
}

void rollback_transaction(database::IDatabaseDriver* driver, std::string* error_message) {
    if (!driver) { return; }
    (void)driver->execute("ROLLBACK", error_message);
}

std::string build_oracle_temporal_literal(const database::ColumnTypeFamily family, const std::string& converted_value) {
    if (family == database::ColumnTypeFamily::DateTime) {
        return "TO_TIMESTAMP('" + sql_escape_single_quote(converted_value) + "', 'YYYY-MM-DD HH24:MI:SS')";
    }
    if (family == database::ColumnTypeFamily::Date) {
        return "TO_DATE('" + sql_escape_single_quote(converted_value) + "', 'YYYY-MM-DD')";
    }
    if (family == database::ColumnTypeFamily::Time) {
        return "TO_TIMESTAMP('" + sql_escape_single_quote(converted_value) + "', 'HH24:MI:SS')";
    }
    return "'" + sql_escape_single_quote(converted_value) + "'";
}

std::string truncate_for_log(const std::string& text, const std::size_t max_size) {
    if (text.size() <= max_size) { return text; }
    return text.substr(0, max_size) + "...<truncated>";
}

bool is_load_data_disabled_error(const std::string& error_text) {
    const std::string lower = [&] {
        std::string text = error_text;
        std::transform(text.begin(), text.end(), text.begin(), [](const unsigned char ch) {
            return static_cast<char>(std::tolower(ch));
        });
        return text;
    }();
    return lower.find("local data is disabled") != std::string::npos || lower.find("local_infile") != std::string::npos;
}

std::string trim_ascii_spaces(std::string text) {
    std::size_t start = 0;
    while (start < text.size() && std::isspace(static_cast<unsigned char>(text[start]))) { ++start; }

    std::size_t end = text.size();
    while (end > start && std::isspace(static_cast<unsigned char>(text[end - 1]))) { --end; }

    return text.substr(start, end - start);
}

std::string to_upper_ascii(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](const unsigned char ch) {
        return static_cast<char>(std::toupper(ch));
    });
    return text;
}

std::unordered_map<std::string, std::string> parse_odbc_attributes(const std::string& connection_string) {
    std::unordered_map<std::string, std::string> attributes;
    std::size_t                                  begin = 0;
    while (begin <= connection_string.size()) {
        const std::size_t end = connection_string.find(';', begin);
        const std::string token =
            end == std::string::npos ? connection_string.substr(begin) : connection_string.substr(begin, end - begin);
        if (!token.empty()) {
            const std::size_t equal = token.find('=');
            if (equal != std::string::npos) {
                const std::string key   = to_upper_ascii(trim_ascii_spaces(token.substr(0, equal)));
                const std::string value = trim_ascii_spaces(token.substr(equal + 1));
                if (!key.empty()) { attributes[key] = value; }
            }
        }
        if (end == std::string::npos) { break; }
        begin = end + 1;
    }
    return attributes;
}

std::optional<std::filesystem::path> resolve_odbc_ini_path() {
    if (const auto odbc_ini = utils::get_env_value("ODBCINI"); odbc_ini.has_value()) {
        return std::filesystem::path(*odbc_ini);
    }
    if (const auto odbc_sys = utils::get_env_value("ODBCSYSINI"); odbc_sys.has_value()) {
        return std::filesystem::path(*odbc_sys) / "odbc.ini";
    }

    const std::vector<std::filesystem::path> candidates = {
        "/etc/odbc.ini",
        "/usr/local/etc/odbc.ini",
        "/opt/homebrew/etc/odbc.ini",
    };
    for (const auto& path : candidates) {
        if (std::filesystem::exists(path)) { return path; }
    }

    if (const auto home = utils::get_env_value("HOME"); home.has_value()) {
        std::filesystem::path user_path = std::filesystem::path(*home) / ".odbc.ini";
        if (std::filesystem::exists(user_path)) { return user_path; }
    }

    return std::nullopt;
}

bool load_odbc_ini_section(const std::string& section_name, std::unordered_map<std::string, std::string>& out) {
    const auto path = resolve_odbc_ini_path();
    if (!path.has_value()) { return false; }

    std::ifstream in(*path);
    if (!in) { return false; }

    const std::string target = to_upper_ascii(trim_ascii_spaces(section_name));
    std::string       current;
    std::string       line;
    bool              found = false;

    while (std::getline(in, line)) {
        std::string trimmed = trim_ascii_spaces(line);
        if (trimmed.empty()) { continue; }
        if (trimmed[0] == ';' || trimmed[0] == '#') { continue; }

        if (trimmed.front() == '[' && trimmed.back() == ']') {
            current = to_upper_ascii(trim_ascii_spaces(trimmed.substr(1, trimmed.size() - 2)));
            continue;
        }

        if (current != target) { continue; }
        const std::size_t equal = trimmed.find('=');
        if (equal == std::string::npos) { continue; }

        const std::string key   = to_upper_ascii(trim_ascii_spaces(trimmed.substr(0, equal)));
        const std::string value = trim_ascii_spaces(trimmed.substr(equal + 1));
        if (!key.empty()) {
            out[key] = value;
            found    = true;
        }
    }

    return found;
}

void parse_oracle_dbq(const std::string& dbq, std::string* host, std::string* port, std::string* database) {
    const std::size_t colon = dbq.find(':');
    const std::size_t slash = dbq.find('/');
    if (colon != std::string::npos) {
        *host = dbq.substr(0, colon);
        if (slash != std::string::npos && slash > colon + 1) {
            *port = dbq.substr(colon + 1, slash - colon - 1);
        } else if (colon + 1 < dbq.size()) {
            *port = dbq.substr(colon + 1);
        }
    } else if (!dbq.empty()) {
        *host = dbq;
    }
    if (slash != std::string::npos && slash + 1 < dbq.size()) { *database = dbq.substr(slash + 1); }
}

std::string build_connection_info_line(const database::DbUrl& db_url) {
    if (db_url.type == database::DbType::Sqlite) { return "Database connection: PATH=" + db_url.database; }
    std::string driver;
    std::string dsn;
    std::string host     = db_url.host;
    std::string port     = db_url.port > 0 ? std::to_string(db_url.port) : "";
    std::string database = db_url.database;
    std::string user     = db_url.username;

    const auto attributes = parse_odbc_attributes(db_url.odbc_connection_string);
    if (const auto it = attributes.find("DRIVER"); it != attributes.end()) { driver = it->second; }
    if (const auto it = attributes.find("DSN"); it != attributes.end()) { dsn = it->second; }
    if (host.empty()) {
        if (const auto it = attributes.find("SERVER"); it != attributes.end()) { host = it->second; }
    }
    if (host.empty()) {
        if (const auto it = attributes.find("HOST"); it != attributes.end()) { host = it->second; }
    }
    if (port.empty()) {
        if (const auto it = attributes.find("PORT"); it != attributes.end()) { port = it->second; }
    }
    if (database.empty()) {
        if (const auto it = attributes.find("DATABASE"); it != attributes.end()) { database = it->second; }
    }
    if (database.empty()) {
        if (const auto it = attributes.find("DB"); it != attributes.end()) { database = it->second; }
    }
    if (user.empty()) {
        if (const auto it = attributes.find("UID"); it != attributes.end()) { user = it->second; }
    }
    if (user.empty()) {
        if (const auto it = attributes.find("USER"); it != attributes.end()) { user = it->second; }
    }
    if (user.empty()) {
        if (const auto it = attributes.find("USERNAME"); it != attributes.end()) { user = it->second; }
    }
    if (user.empty()) {
        if (const auto it = attributes.find("USERID"); it != attributes.end()) { user = it->second; }
    }
    if (user.empty()) {
        if (const auto it = attributes.find("USER ID"); it != attributes.end()) { user = it->second; }
    }

    if (const auto it = attributes.find("DBQ"); it != attributes.end()) {
        parse_oracle_dbq(it->second, &host, &port, &database);
    }

    if (!dsn.empty()) {
        std::unordered_map<std::string, std::string> ini_attributes;
        if (load_odbc_ini_section(dsn, ini_attributes)) {
            if (driver.empty()) {
                if (const auto it = ini_attributes.find("DRIVER"); it != ini_attributes.end()) { driver = it->second; }
            }
            if (host.empty()) {
                if (const auto it = ini_attributes.find("SERVER"); it != ini_attributes.end()) { host = it->second; }
            }
            if (host.empty()) {
                if (const auto it = ini_attributes.find("HOST"); it != ini_attributes.end()) { host = it->second; }
            }
            if (host.empty()) {
                if (const auto it = ini_attributes.find("SERVERNAME"); it != ini_attributes.end()) {
                    host = it->second;
                }
            }
            if (port.empty()) {
                if (const auto it = ini_attributes.find("PORT"); it != ini_attributes.end()) { port = it->second; }
            }
            if (database.empty()) {
                if (const auto it = ini_attributes.find("DATABASE"); it != ini_attributes.end()) {
                    database = it->second;
                }
            }
            if (database.empty()) {
                if (const auto it = ini_attributes.find("DB"); it != ini_attributes.end()) { database = it->second; }
            }
            if (user.empty()) {
                if (const auto it = ini_attributes.find("UID"); it != ini_attributes.end()) { user = it->second; }
            }
            if (user.empty()) {
                if (const auto it = ini_attributes.find("USER"); it != ini_attributes.end()) { user = it->second; }
            }
            if (user.empty()) {
                if (const auto it = ini_attributes.find("USERNAME"); it != ini_attributes.end()) { user = it->second; }
            }
            if (user.empty()) {
                if (const auto it = ini_attributes.find("USERID"); it != ini_attributes.end()) { user = it->second; }
            }
            if (user.empty()) {
                if (const auto it = ini_attributes.find("USER ID"); it != ini_attributes.end()) { user = it->second; }
            }
            if (const auto it = ini_attributes.find("DBQ"); it != ini_attributes.end()) {
                parse_oracle_dbq(it->second, &host, &port, &database);
            }
        }
    }

    std::vector<std::string> parts;
    if (!dsn.empty()) { parts.push_back("DSN=" + dsn); }
    if (!driver.empty()) { parts.push_back("DRIVER=" + driver); }
    if (!user.empty()) { parts.push_back("USER=" + user); }
    if (!host.empty()) { parts.push_back("HOST=" + host); }
    if (!port.empty()) { parts.push_back("PORT=" + port); }
    if (!database.empty()) { parts.push_back("DATABASE=" + database); }

    if (parts.empty()) { return "Database connection: <unavailable>"; }

    std::ostringstream oss;
    oss << "Database connection: ";
    for (std::string::size_type i = 0; i < parts.size(); ++i) {
        if (i > 0) { oss << ", "; }
        oss << parts[i];
    }
    return oss.str();
}

std::string build_generated_line(const std::uint64_t generated, const std::uint64_t total) {
    return "Rows Generated: " + logging::format_progress_bar(generated, total);
}

std::string build_imported_line(const std::uint64_t imported, const std::uint64_t total) {
    return "Data Imported: " + logging::format_progress_bar(imported, total);
}

std::string format_elapsed_seconds(const double elapsed_seconds) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(3) << elapsed_seconds << "s";
    return oss.str();
}

}  // namespace

OutputStats DatabaseBackend::generate(const config::GenerationConfig& cfg, const engine::ExecutionOptions& options) {
    logging::Logger& logger = logging::Logger::instance();

    auto format_config_line = [](const std::string& label, const std::string& value, const std::string& note) {
        if (note.empty()) { return label + ": " + value; }
        return label + ": " + value + " (" + note + ")";
    };

    auto log_field_lines = [&](logging::Logger* current_logger) {
        if (!current_logger) { return; }
        for (const auto& field : cfg.fields) { current_logger->info(build_field_log_line(field)); }
    };

    database::DbUrl db_url;
    std::string     error;
    if (!database::parse_db_connection(cfg.output.database.connection, &db_url, &error)) {
        logger.error("Run failed: invalid database connection: " + error);
        throw std::runtime_error("invalid database connection: " + error);
    }

    auto driver = database::make_database_driver(db_url.type);
    if (!driver) {
        const std::string message = "unsupported database type: " + database::db_type_to_string(db_url.type);
        logger.error("Run failed: " + message);
        throw std::runtime_error(message);
    }
    if (!driver->connect(db_url, &error)) {
        logger.error("Run failed: database connection failed: " + error);
        throw std::runtime_error("database connection failed: " + error);
    }

    const std::string table_name = cfg.output.database.table;
    if (table_name.empty()) {
        logger.error("Run failed: database output requires table name");
        throw std::runtime_error("database output requires table name");
    }

    database::TableMetadata metadata;
    if (!driver->get_table_metadata(table_name, &metadata, &error)) {
        logger.error("Run failed: failed to load table metadata: " + error);
        throw std::runtime_error("failed to load table metadata: " + error);
    }

    const database::DbType db_type = driver->type();

    std::unordered_map<std::string, const database::ColumnMetadata*> column_map;
    for (const auto& column : metadata.columns) { column_map[column.name] = &column; }

    std::vector<std::string> columns;
    columns.reserve(cfg.fields.size());
    std::vector<std::string> sql_columns;
    sql_columns.reserve(cfg.fields.size());
    std::vector<const database::ColumnMetadata*> mapped_columns;
    mapped_columns.reserve(cfg.fields.size());
    for (const auto& field : cfg.fields) {
        if (!column_map.contains(field.name)) {
            logger.error("Run failed: column not found in metadata: " + field.name);
            throw std::runtime_error("column not found in metadata: " + field.name);
        }
        columns.push_back(field.name);
        sql_columns.push_back(quote_identifier_path(db_type, field.name));
        mapped_columns.push_back(column_map.at(field.name));
    }
    const std::string sql_table_name = quote_identifier_path(db_type, table_name);

    config::InsertMode resolved_mode = choose_insert_mode(cfg);
    std::string        insert_mode_note;
    if (cfg.output.database.write_mode == config::WriteMode::Upsert && resolved_mode == config::InsertMode::Load) {
        resolved_mode     = config::InsertMode::Insert;
        insert_mode_note  = "requested load, upsert requires insert";
    } else if (resolved_mode == config::InsertMode::Load && !driver->supports_load_mode()) {
        resolved_mode     = config::InsertMode::Insert;
        insert_mode_note  = "requested load, current database does not support load mode";
    }

    config::ErrorPolicy effective_error_policy = cfg.output.database.error_policy;
    std::string         error_policy_note;
    if (effective_error_policy == config::ErrorPolicy::Rollback && resolved_mode != config::InsertMode::Insert) {
        effective_error_policy = config::ErrorPolicy::Stop;
        error_policy_note      = "requested rollback, rollback only works with insert_mode=insert";
    }

    const int total_rows        = cfg.rows;
    const int batch_size        = std::max(1, cfg.output.database.batch_size);
    const int queue_size        = std::max(1, cfg.output.database.queue_size);
    const int requested_threads = std::max(1, cfg.output.database.db_threads);
    int       db_threads        = requested_threads;
    std::string threads_note;

    if (cfg.output.database.transaction_mode == config::TransactionMode::PerRun && db_threads > 1) {
        db_threads   = 1;
        threads_note = "requested " + std::to_string(requested_threads) + ", per-run transaction uses one writer";
    }
    if (db_type == database::DbType::Sqlite && db_threads > 1) {
        db_threads   = 1;
        threads_note = "requested " + std::to_string(requested_threads) + ", SQLite supports a single writer";
    }

    logger.info("Output type: database");
    logger.info(build_connection_info_line(db_url));
    logger.info("Connection established: " + driver->dbms_name() + " " + driver->dbms_version());
    logger.info("Table: " + table_name);
    logger.info(format_config_line("Mode", config::database_mode_to_string(cfg.output.database.mode), ""));
    logger.info(format_config_line("Write mode", config::write_mode_to_string(cfg.output.database.write_mode), ""));
    logger.info(format_config_line(
        "Error policy",
        config::error_policy_to_string(effective_error_policy),
        error_policy_note
    ));
    logger.info(format_config_line(
        "Transaction mode",
        config::transaction_mode_to_string(cfg.output.database.transaction_mode),
        ""
    ));
    logger.info("Advanced parameters:");
    logger.info(format_config_line("Insert mode", config::insert_mode_to_string(resolved_mode), insert_mode_note));
    logger.info(format_config_line("Batch size", std::to_string(batch_size), ""));
    logger.info(format_config_line("Queue size", std::to_string(queue_size), ""));
    logger.info(format_config_line("Threads", std::to_string(db_threads), threads_note));
    logger.info(format_config_line(
        "Rate limit rows per sec",
        std::to_string(cfg.output.database.rate_limit_rows_per_sec),
        cfg.output.database.rate_limit_rows_per_sec == 0 ? "0 means unlimited" : ""
    ));

    const database::SchemaValidationReport report = database::validate_table_schema(cfg, metadata);
    for (const auto& message : report.messages) {
        if (message.level == database::ValidationLevel::Warn) { logger.warn(message.message); }
        if (message.level == database::ValidationLevel::Error) { logger.error(message.message); }
    }
    if (report.error_count() > 0) { throw std::runtime_error("table schema validation failed"); }

    if (cfg.output.database.write_mode == config::WriteMode::Truncate) {
        if (!driver->execute("DELETE FROM " + sql_table_name, &error)) {
            logger.error("Run failed: failed to clear table before generation: " + error);
            throw std::runtime_error("failed to clear table before generation: " + error);
        }
        logger.info("Table cleared: " + table_name);
    }

    std::optional<std::vector<std::size_t>> upsert_key_indexes;
    if (cfg.output.database.write_mode == config::WriteMode::Upsert) {
        upsert_key_indexes = determine_upsert_key_indexes(metadata, columns, mapped_columns);
        if (!upsert_key_indexes.has_value()) {
            logger.error("Run failed: upsert requires at least one generated primary key or unique key");
            throw std::runtime_error("upsert requires at least one generated primary key or unique key");
        }
    }

    log_field_lines(&logger);
    logger.info("Generation started");

    std::atomic                        stop{false};
    std::atomic                        rollback_all{false};
    std::atomic                        load_fallback_warned{false};
    std::atomic<std::uint64_t>         rows_generated{0};
    std::atomic<std::uint64_t>         rows_imported{0};
    std::vector<std::filesystem::path> temp_files;
    std::mutex                         temp_files_mutex;
    BoundedQueue<RowWithIndex>         queue(static_cast<std::size_t>(queue_size));

    std::mutex  error_mutex;
    std::string first_error;

    std::mutex  progress_mutex;
    std::atomic progress_active{false};
    const bool  multiline_progress = stdout_is_tty();
    bool        progress_rendered  = false;

    auto clear_progress_line = [&] {
        if (!progress_rendered) { return; }
        if (multiline_progress) {
            std::cout << "\r" << kAnsiClearLine << kAnsiMoveUp << "\r" << kAnsiClearLine << "\r" << std::flush;
        } else {
            std::cout << "\r" << std::string(kProgressClearWidth, ' ') << "\r" << std::flush;
        }
        progress_rendered = false;
    };

    auto render_progress = [&](const std::uint64_t generated, const std::uint64_t imported, const bool force) {
        if (!progress_active.load() && !force) { return; }
        if (multiline_progress) {
            if (progress_rendered) {
                std::cout << "\r" << kAnsiClearLine << kAnsiMoveUp << "\r" << kAnsiClearLine << "\r";
            }
            std::cout << build_generated_line(generated, static_cast<std::uint64_t>(total_rows)) << "\n"
                      << build_imported_line(imported, static_cast<std::uint64_t>(total_rows)) << std::flush;
        } else {
            std::cout << "\r" << build_generated_line(generated, static_cast<std::uint64_t>(total_rows)) << " "
                      << build_imported_line(imported, static_cast<std::uint64_t>(total_rows)) << std::flush;
        }
        progress_rendered = true;
    };

    auto log_with_progress = [&](const std::string& message, const auto& log_fn) {
        std::lock_guard lock(progress_mutex);
        if (progress_active.load()) { clear_progress_line(); }
        log_fn(message);
        if (progress_active.load()) { render_progress(rows_generated.load(), rows_imported.load(), false); }
    };

    auto register_error = [&](const std::string& text) {
        std::lock_guard lock(error_mutex);
        if (first_error.empty()) { first_error = text; }
        log_with_progress(text, [&](const std::string& message) { logger.error(message); });
    };

    auto should_continue_after_error = [&] {
        if (effective_error_policy == config::ErrorPolicy::Continue) { return true; }
        if (effective_error_policy == config::ErrorPolicy::Rollback &&
            cfg.output.database.transaction_mode == config::TransactionMode::PerBatch) {
            return true;
        }
        return false;
    };

    auto handle_policy = [&](const std::string& context_error) {
        if (effective_error_policy == config::ErrorPolicy::Continue) {
            log_with_progress(context_error + " (continue)", [&](const std::string& message) { logger.warn(message); });
            return;
        }
        if (effective_error_policy == config::ErrorPolicy::Rollback) {
            if (cfg.output.database.transaction_mode == config::TransactionMode::PerBatch) {
                log_with_progress(context_error + " (rollback current batch and continue)", [&](const std::string& message) {
                    logger.warn(message);
                });
                return;
            }
            if (cfg.output.database.transaction_mode == config::TransactionMode::PerRun) {
                rollback_all.store(true);
                log_with_progress(context_error + " (rollback full run)", [&](const std::string& message) {
                    logger.warn(message);
                });
            }
        }
        stop.store(true);
    };

    std::atomic progress_stop{false};
    progress_active.store(true);
    std::thread progress_thread([&] {
        while (!progress_stop.load()) {
            {
                std::lock_guard lock(progress_mutex);
                render_progress(rows_generated.load(), rows_imported.load(), false);
            }
            std::this_thread::sleep_for(std::chrono::duration<double>(kProgressSleepSeconds));
        }
    });

    const auto started_at = std::chrono::steady_clock::now();
    auto       generation_finished_at = started_at;

    std::vector<std::thread> workers;
    workers.reserve(static_cast<std::size_t>(db_threads));

    for (int worker_id = 0; worker_id < db_threads; ++worker_id) {
        workers.emplace_back([&, worker_id] {
            auto worker_driver = database::make_database_driver(db_url.type);
            if (!worker_driver) {
                register_error("failed to create database driver for worker");
                stop.store(true);
                return;
            }

            std::string worker_error;
            if (!worker_driver->connect(db_url, &worker_error)) {
                register_error("worker connection failed: " + worker_error);
                stop.store(true);
                return;
            }

            const database::DefaultTypeAdapter adapter;
            const bool per_run_tx = cfg.output.database.transaction_mode == config::TransactionMode::PerRun;
            if (per_run_tx && !begin_transaction(db_type, worker_driver.get(), &worker_error)) {
                register_error("worker failed to start transaction: " + worker_error);
                stop.store(true);
                return;
            }

            std::vector<RowWithIndex> batch;
            batch.reserve(static_cast<std::size_t>(batch_size));

            auto process_batch = [&](const std::vector<RowWithIndex>& batch_rows) {
                if (batch_rows.empty()) { return true; }

                std::vector<std::vector<std::string>> sql_rows;
                sql_rows.reserve(batch_rows.size());

                for (const auto& [row_index, row] : batch_rows) {
                    std::vector<std::string> sql_values;
                    sql_values.reserve(mapped_columns.size());

                    for (std::string::size_type i = 0; i < mapped_columns.size(); ++i) {
                        const auto adapted = adapter.adapt(*mapped_columns[i], row[i]);
                        if (!adapted.ok) {
                            std::ostringstream oss;
                            oss << "Type conversion failed: row=" << row_index << ", field=" << cfg.fields[i].name
                                << ", raw=" << (row[i].has_value() ? *row[i] : "<NULL>")
                                << ", message=" << adapted.error_message;
                            register_error(oss.str());
                            handle_policy("Type conversion failed");
                            return should_continue_after_error();
                        }

                        std::string sql_literal = adapted.sql_literal;
                        if (db_type == database::DbType::Oracle && !adapted.is_null) {
                            const database::ColumnTypeFamily family =
                                database::classify_column_type(*mapped_columns[i]);
                            if (family == database::ColumnTypeFamily::DateTime ||
                                family == database::ColumnTypeFamily::Date ||
                                family == database::ColumnTypeFamily::Time) {
                                sql_literal = build_oracle_temporal_literal(family, adapted.converted_value);
                            }
                        }
                        sql_values.push_back(std::move(sql_literal));
                    }

                    sql_rows.push_back(std::move(sql_values));
                }

                auto execute_sql = [&](const std::string& sql) {
                    if (sql.empty()) {
                        register_error("upsert is not supported for the current database type");
                        stop.store(true);
                        return false;
                    }
                    if (!worker_driver->execute(sql, &worker_error)) {
                        register_error("SQL execution failed: " + worker_error + " sql=" + truncate_for_log(sql, 4096));
                        handle_policy("SQL execution failed");
                        return false;
                    }
                    return true;
                };

                const bool per_batch_tx = cfg.output.database.transaction_mode == config::TransactionMode::PerBatch;
                if (per_batch_tx && !begin_transaction(db_type, worker_driver.get(), &worker_error)) {
                    register_error("failed to start batch transaction: " + worker_error);
                    stop.store(true);
                    return false;
                }

                bool ok = true;
                if (cfg.output.database.write_mode == config::WriteMode::Upsert) {
                    ok = execute_sql(build_upsert_sql(
                        db_type,
                        sql_table_name,
                        sql_columns,
                        *upsert_key_indexes,
                        sql_rows
                    ));
                } else if (resolved_mode == config::InsertMode::Insert) {
                    ok = execute_sql(build_insert_sql(db_type, sql_table_name, sql_columns, sql_rows));
                } else {
                    const std::filesystem::path load_file =
                        std::filesystem::path(cfg.workspace) /
                        "tmp" /
                        ("load_" + std::to_string(worker_id) + "_" + std::to_string(batch_rows.front().first) + ".tmp");
                    std::ofstream temp(load_file, std::ios::trunc);
                    if (!temp) {
                        register_error("failed to open temp load file: " + load_file.string());
                        stop.store(true);
                        ok = false;
                    } else {
                        for (const auto& row_values : sql_rows) { temp << render_row_for_load(row_values); }
                        temp.close();
                        {
                            std::lock_guard lock(temp_files_mutex);
                            temp_files.push_back(load_file);
                        }

                        const std::string load_sql =
                            build_load_sql(db_type, sql_table_name, sql_columns, load_file.string());
                        if (load_sql.empty()) {
                            register_error("load mode is not supported for current database type");
                            handle_policy("Load mode is not supported");
                            ok = false;
                        } else if (!worker_driver->execute(load_sql, &worker_error)) {
                            const bool can_fallback_to_insert =
                                db_type == database::DbType::Mysql && is_load_data_disabled_error(worker_error);
                            if (can_fallback_to_insert) {
                                bool expected = false;
                                if (load_fallback_warned.compare_exchange_strong(expected, true)) {
                                    log_with_progress(
                                        "Insert mode fallback: requested load, using insert because LOAD DATA LOCAL INFILE is disabled",
                                        [&](const std::string& message) { logger.warn(message); }
                                    );
                                }
                                ok = execute_sql(build_insert_sql(db_type, sql_table_name, sql_columns, sql_rows));
                            } else {
                                register_error(
                                    "SQL execution failed: " + worker_error + " sql=" + truncate_for_log(load_sql, 4096)
                                );
                                handle_policy("SQL execution failed");
                                ok = false;
                            }
                        }
                    }
                }

                if (per_batch_tx) {
                    if (ok) {
                        if (!commit_transaction(worker_driver.get(), &worker_error)) {
                            register_error("commit failed: " + worker_error);
                            stop.store(true);
                            return false;
                        }
                    } else {
                        rollback_transaction(worker_driver.get(), &worker_error);
                    }
                }

                if (ok) {
                    rows_imported.fetch_add(batch_rows.size());
                    const int rate_limit = cfg.output.database.rate_limit_rows_per_sec;
                    if (rate_limit > 0) {
                        const double seconds = static_cast<double>(batch_rows.size()) / static_cast<double>(rate_limit);
                        const auto   wait_ms = static_cast<int>(seconds * 1000.0);
                        if (wait_ms > 0) { std::this_thread::sleep_for(std::chrono::milliseconds(wait_ms)); }
                    }
                }

                return ok;
            };

            while (!stop.load()) {
                RowWithIndex item;
                if (!queue.pop(&item, &stop)) { break; }
                batch.push_back(std::move(item));

                if (static_cast<int>(batch.size()) >= batch_size) {
                    const bool ok = process_batch(batch);
                    batch.clear();
                    if (!ok && !should_continue_after_error()) { break; }
                    if (stop.load()) { break; }
                }
            }

            if (!batch.empty() && !stop.load()) { (void)process_batch(batch); }

            if (per_run_tx) {
                if (rollback_all.load() || stop.load()) {
                    rollback_transaction(worker_driver.get(), &worker_error);
                } else if (!commit_transaction(worker_driver.get(), &worker_error)) {
                    register_error("per-run commit failed: " + worker_error);
                    stop.store(true);
                }
            }

            worker_driver->disconnect();
        });
    }

    engine::GenerateResult generate_result;
    try {
        generate_result = engine::generate_with_consumer(cfg, options, [&](engine::Row&& row, std::uint64_t row_index) {
            if (stop.load()) { return false; }
            const bool pushed = queue.push(RowWithIndex{row_index, std::move(row)}, &stop);
            if (pushed) { rows_generated.fetch_add(1); }
            return pushed;
        });
    } catch (const std::exception& ex) {
        register_error(std::string("generator failed: ") + ex.what());
        stop.store(true);
    }
    generation_finished_at = std::chrono::steady_clock::now();

    queue.close();
    for (auto& worker : workers) { worker.join(); }

    progress_stop.store(true);
    if (progress_thread.joinable()) { progress_thread.join(); }

    for (const auto& path : temp_files) {
        std::error_code ec;
        std::filesystem::remove(path, ec);
    }

    const auto ended_at    = std::chrono::steady_clock::now();
    double     elapsed_sec = std::chrono::duration<double>(ended_at - started_at).count();
    if (elapsed_sec < kMinElapsedSeconds) { elapsed_sec = kMinElapsedSeconds; }
    double generation_elapsed_sec = std::chrono::duration<double>(generation_finished_at - started_at).count();
    double import_drain_elapsed_sec = std::chrono::duration<double>(ended_at - generation_finished_at).count();
    if (generation_elapsed_sec < 0.0) { generation_elapsed_sec = 0.0; }
    if (import_drain_elapsed_sec < 0.0) { import_drain_elapsed_sec = 0.0; }

    const std::uint64_t generated = rows_generated.load();
    const std::uint64_t imported  = rows_imported.load();
    {
        std::lock_guard lock(progress_mutex);
        render_progress(generated, imported, true);
        std::cout << "\n" << std::flush;
    }
    progress_active.store(false);

    logger.info(
        "Elapsed: generation=" +
        format_elapsed_seconds(generation_elapsed_sec) +
        ", import=" +
        format_elapsed_seconds(elapsed_sec) +
        ", drain=" +
        format_elapsed_seconds(import_drain_elapsed_sec)
    );

    const std::uint64_t generated_failed = generated <= static_cast<std::uint64_t>(total_rows)
                                               ? static_cast<std::uint64_t>(total_rows) - generated
                                               : 0;
    const std::uint64_t imported_failed  = generated >= imported ? generated - imported : 0;
    const auto          generated_rate =
        static_cast<std::uint64_t>(static_cast<double>(generated) / elapsed_sec);
    const auto imported_rate = static_cast<std::uint64_t>(static_cast<double>(imported) / elapsed_sec);

    logger.info(
        "Generated rows: total=" +
        std::to_string(generated) +
        ", success=" +
        std::to_string(generated) +
        ", failed=" +
        std::to_string(generated_failed) +
        ", rate=" +
        std::to_string(generated_rate) +
        " rows/s"
    );
    logger.info(
        "Imported rows: total=" +
        std::to_string(imported) +
        ", success=" +
        std::to_string(imported) +
        ", failed=" +
        std::to_string(imported_failed) +
        ", rate=" +
        std::to_string(imported_rate) +
        " rows/s"
    );
    logger.info("Completed");

    if (!first_error.empty() && stop.load()) { throw std::runtime_error(first_error); }

    OutputStats stats;
    stats.execution_info = generate_result.info;
    stats.rows_generated = generated;
    stats.rows_written   = imported;
    return stats;
}

}  // namespace datagen::output
