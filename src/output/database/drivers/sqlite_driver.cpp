// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file sqlite_driver.cpp

#include "output/database/drivers/sqlite_driver.h"

#include <algorithm>
#include <cctype>
#include <optional>
#include <sstream>

namespace data_generator::database {

namespace {

std::string to_lower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return text;
}

std::string escape_sql_literal(const std::string& value) {
    std::string escaped;
    escaped.reserve(value.size());
    for (const char ch : value) {
        if (ch == '\'') { escaped.push_back('\''); }
        escaped.push_back(ch);
    }
    return escaped;
}

std::string quote_identifier(const std::string& value) {
    std::string escaped;
    escaped.reserve(value.size() + 2);
    for (const char ch : value) {
        escaped.push_back(ch);
        if (ch == '"') { escaped.push_back('"'); }
    }
    return "\"" + escaped + "\"";
}

std::optional<int> parse_int(const std::string& value) {
    if (value.empty()) { return std::nullopt; }
    try {
        return std::stoi(value);
    } catch (...) {
        return std::nullopt;
    }
}

void parse_type_details(
    const std::string& type_name,
    std::optional<int>& character_length,
    std::optional<int>& numeric_precision,
    std::optional<int>& numeric_scale
) {
    character_length.reset();
    numeric_precision.reset();
    numeric_scale.reset();

    const std::size_t open = type_name.find('(');
    const std::size_t close = type_name.find(')', open);
    if (open == std::string::npos || close == std::string::npos || close <= open + 1) { return; }

    const std::string inside = type_name.substr(open + 1, close - open - 1);
    const std::size_t comma = inside.find(',');
    if (comma == std::string::npos) {
        const auto parsed = parse_int(inside);
        if (!parsed.has_value()) { return; }
        const std::string lower = to_lower(type_name);
        if (lower.find("char") != std::string::npos || lower.find("clob") != std::string::npos ||
            lower.find("text") != std::string::npos) {
            character_length = parsed;
        } else {
            numeric_precision = parsed;
        }
        return;
    }

    const auto precision = parse_int(inside.substr(0, comma));
    const auto scale = parse_int(inside.substr(comma + 1));
    if (precision.has_value()) { numeric_precision = precision; }
    if (scale.has_value()) { numeric_scale = scale; }
}

}  // namespace

SqliteDriver::~SqliteDriver() {
    disconnect();
}

DbType SqliteDriver::type() const {
    return DbType::Sqlite;
}

std::string SqliteDriver::dbms_name() const {
    return "SQLite";
}

std::string SqliteDriver::dbms_version() const {
    return sqlite3_libversion();
}

bool SqliteDriver::connect(const DbUrl& url, std::string* error_message) {
    disconnect();

    if (url.type != DbType::Sqlite) {
        if (error_message) { *error_message = "sqlite driver requires sqlite connection"; }
        return false;
    }
    if (url.database.empty()) {
        if (error_message) { *error_message = "sqlite connection missing path"; }
        return false;
    }

    db_path_ = url.database;
    const int rc = sqlite3_open_v2(
        db_path_.c_str(),
        &db_,
        SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
        nullptr
    );
    if (rc != SQLITE_OK) {
        if (error_message) { *error_message = sqlite3_errmsg(db_ ? db_ : nullptr); }
        disconnect();
        return false;
    }

    connected_ = true;
    return true;
}

void SqliteDriver::disconnect() {
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
    connected_ = false;
    db_path_.clear();
}

bool SqliteDriver::test_connection(std::string* error_message) {
    std::vector<std::vector<std::string>> rows;
    if (!run_query("SELECT 1", &rows, error_message)) { return false; }
    return !rows.empty();
}

bool SqliteDriver::execute(const std::string& sql, std::string* error_message) {
    if (!connected_) {
        if (error_message) { *error_message = "driver is not connected"; }
        return false;
    }
    char* error_text = nullptr;
    const int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &error_text);
    if (rc != SQLITE_OK) {
        if (error_message) {
            *error_message = error_text ? error_text : sqlite3_errmsg(db_);
        }
        if (error_text) { sqlite3_free(error_text); }
        return false;
    }
    return true;
}

bool SqliteDriver::query(
    const std::string&                     sql,
    std::vector<std::vector<std::string>>* rows,
    std::string*                           error_message
) {
    if (!rows) {
        if (error_message) { *error_message = "rows output pointer is null"; }
        return false;
    }
    if (!connected_) {
        if (error_message) { *error_message = "driver is not connected"; }
        return false;
    }
    return run_query(sql, rows, error_message);
}

bool SqliteDriver::get_table_metadata(
    const std::string& table_name,
    TableMetadata*     metadata,
    std::string*       error_message
) {
    if (!metadata) {
        if (error_message) { *error_message = "metadata output pointer is null"; }
        return false;
    }
    if (!connected_) {
        if (error_message) { *error_message = "driver is not connected"; }
        return false;
    }
    return load_table_metadata(table_name, metadata, error_message);
}

bool SqliteDriver::supports_load_mode() const {
    return false;
}

bool SqliteDriver::run_query(
    const std::string&                     sql,
    std::vector<std::vector<std::string>>* rows,
    std::string*                           error_message
) const {
    rows->clear();

    sqlite3_stmt* stmt = nullptr;
    const int prepare_rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
    if (prepare_rc != SQLITE_OK) {
        if (error_message) { *error_message = sqlite3_errmsg(db_); }
        return false;
    }

    const int column_count = sqlite3_column_count(stmt);
    for (int step_rc = sqlite3_step(stmt); step_rc != SQLITE_DONE; step_rc = sqlite3_step(stmt)) {
        if (step_rc != SQLITE_ROW) {
            if (error_message) { *error_message = sqlite3_errmsg(db_); }
            sqlite3_finalize(stmt);
            return false;
        }

        std::vector<std::string> row;
        row.reserve(static_cast<std::size_t>(column_count));
        for (int col = 0; col < column_count; ++col) {
            if (sqlite3_column_type(stmt, col) == SQLITE_NULL) {
                row.emplace_back();
                continue;
            }
            const unsigned char* text = sqlite3_column_text(stmt, col);
            row.emplace_back(text ? reinterpret_cast<const char*>(text) : "");
        }
        rows->push_back(std::move(row));
    }

    sqlite3_finalize(stmt);
    return true;
}

bool SqliteDriver::load_table_metadata(
    const std::string& table_name,
    TableMetadata*     metadata,
    std::string*       error_message
) const {
    metadata->table_name = table_name;
    metadata->columns.clear();
    metadata->foreign_keys.clear();
    metadata->indexes.clear();
    metadata->triggers.clear();
    metadata->partitioned = false;

    const std::string escaped_table = escape_sql_literal(table_name);
    std::vector<std::vector<std::string>> rows;

    std::string create_sql;
    if (run_query(
            "SELECT sql FROM sqlite_master WHERE type='table' AND name='" + escaped_table + "'",
            &rows,
            error_message
        )) {
        if (!rows.empty() && !rows.front().empty()) {
            create_sql = to_lower(rows.front().front());
        }
    }

    rows.clear();
    if (!run_query("PRAGMA table_info('" + escaped_table + "')", &rows, error_message)) { return false; }
    if (rows.empty()) {
        if (error_message) { *error_message = "table not found: " + table_name; }
        return false;
    }

    for (const auto& row : rows) {
        if (row.size() < 6) { continue; }
        ColumnMetadata column;
        column.name = row[1];
        column.data_type = row[2];
        if (!row[4].empty()) { column.default_value = row[4]; }
        const bool is_pk = (row[5] == "1");
        const std::string lower_type = to_lower(row[2]);
        column.auto_increment = is_pk && lower_type.find("int") != std::string::npos;
        if (!create_sql.empty() && create_sql.find("autoincrement") != std::string::npos) {
            column.auto_increment = is_pk && lower_type.find("int") != std::string::npos;
        }
        parse_type_details(lower_type, column.character_length, column.numeric_precision, column.numeric_scale);
        metadata->columns.push_back(std::move(column));
    }

    rows.clear();
    if (!run_query("PRAGMA foreign_key_list('" + escaped_table + "')", &rows, error_message)) { return false; }
    for (const auto& row : rows) {
        if (row.size() < 5) { continue; }
        metadata->foreign_keys.push_back(ForeignKeyMetadata{
            .column_name = row[3],
            .referenced_table = row[2],
            .referenced_column = row[4],
        });
    }

    rows.clear();
    if (!run_query("PRAGMA index_list('" + escaped_table + "')", &rows, error_message)) { return false; }
    for (const auto& row : rows) {
        if (row.size() < 3) { continue; }
        IndexMetadata index;
        index.name = row[1];
        index.unique = (row[2] == "1");

        std::vector<std::vector<std::string>> index_rows;
        if (!run_query("PRAGMA index_info(" + quote_identifier(index.name) + ")", &index_rows, error_message)) {
            return false;
        }
        for (const auto& index_row : index_rows) {
            if (index_row.size() < 3) { continue; }
            index.columns.push_back(index_row[2]);
        }
        metadata->indexes.push_back(std::move(index));
    }

    rows.clear();
    if (!run_query(
            "SELECT name FROM sqlite_master WHERE type='trigger' AND tbl_name='" + escaped_table + "'",
            &rows,
            error_message
        )) {
        return false;
    }
    for (const auto& row : rows) {
        if (row.empty()) { continue; }
        metadata->triggers.push_back(TriggerMetadata{.name = row[0], .event = "trigger"});
    }

    return true;
}

}  // namespace data_generator::database
