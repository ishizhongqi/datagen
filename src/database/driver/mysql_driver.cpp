// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

#include "database/driver/mysql_driver.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>

namespace data_generator::database {

namespace {

#if defined(__APPLE__)
constexpr const char* kMysqlBinary = "/opt/homebrew/opt/mysql@8.0/bin/mysql";
#else
constexpr const char* kMysqlBinary = "mysql";
#endif

std::optional<std::string> read_env_value(const char* key) {
#if defined(_WIN32)
    char*  value  = nullptr;
    size_t length = 0;
    if (_dupenv_s(&value, &length, key) != 0 || value == nullptr) { return std::nullopt; }

    std::string text(value);
    std::free(value);
    return text;
#else
    const char* value = std::getenv(key);
    if (!value) { return std::nullopt; }
    return std::string(value);
#endif
}

#if defined(_WIN32)
std::string shell_escape(const std::string& value) {
    std::string result;
    result.reserve(value.size() + 2);
    result.push_back('"');
    for (const char ch : value) {
        if (ch == '"') {
            result += "\\\"";
        } else if (ch == '%') {
            result += "%%";
        } else {
            result.push_back(ch);
        }
    }
    result.push_back('"');
    return result;
}
#else
std::string shell_escape(const std::string& value) {
    std::string result;
    result.reserve(value.size() + 2);
    result.push_back('\'');
    for (const char ch : value) {
        if (ch == '\'') {
            result += "'\\''";
        } else {
            result.push_back(ch);
        }
    }
    result.push_back('\'');
    return result;
}
#endif

std::string trim_line(const std::string& text) {
    std::size_t begin = 0;
    while (begin < text.size() && (text[begin] == '\n' || text[begin] == '\r' || text[begin] == ' ' || text[begin] == '\t')) {
        ++begin;
    }
    std::size_t end = text.size();
    while (end > begin && (text[end - 1] == '\n' || text[end - 1] == '\r' || text[end - 1] == ' ' || text[end - 1] == '\t')) {
        --end;
    }
    return text.substr(begin, end - begin);
}

std::filesystem::path build_temp_sql_path() {
    std::error_code       ec;
    std::filesystem::path base = std::filesystem::temp_directory_path(ec);
    if (ec || base.empty()) {
        base = std::filesystem::current_path(ec);
        if (ec || base.empty()) { base = "."; }
    }

    const auto token = std::to_string(
        static_cast<unsigned long long>(
            std::chrono::high_resolution_clock::now().time_since_epoch().count()
        )
    );
    return base / ("data_generator_mysql_" + token + ".sql");
}

bool write_temp_sql_file(
    const std::string&       sql,
    std::filesystem::path*   path,
    std::string*             error_message
) {
    if (!path) {
        if (error_message) { *error_message = "temp sql path output pointer is null"; }
        return false;
    }

    *path = build_temp_sql_path();
    std::ofstream out(*path, std::ios::trunc);
    if (!out) {
        if (error_message) { *error_message = "failed to create temp sql file: " + path->string(); }
        return false;
    }

    out << sql;
    if (sql.empty() || sql.back() != ';') { out << ";"; }
    out << "\n";
    out.flush();
    if (!out.good()) {
        if (error_message) { *error_message = "failed to write temp sql file: " + path->string(); }
        out.close();
        std::error_code ec;
        std::filesystem::remove(*path, ec);
        return false;
    }
    return true;
}

}  // namespace

DbType MysqlDriver::type() const {
    return DbType::Mysql;
}

bool MysqlDriver::connect(const DbUrl& url, std::string* error_message) {
    connection_ = url;
    connected_  = true;
    return test_connection(error_message);
}

void MysqlDriver::disconnect() {
    connected_ = false;
}

bool MysqlDriver::test_connection(std::string* error_message) {
    if (!connected_) {
        if (error_message) { *error_message = "driver is not connected"; }
        return false;
    }

    std::string output;
    if (!run_mysql_command("SELECT 1", true, &output, error_message)) { return false; }
    return output.find('1') != std::string::npos;
}

bool MysqlDriver::execute(const std::string& sql, std::string* error_message) {
    std::string output;
    return run_mysql_command(sql, false, &output, error_message);
}

bool MysqlDriver::query(
    const std::string&                     sql,
    std::vector<std::vector<std::string>>* rows,
    std::string*                           error_message
) {
    if (!rows) {
        if (error_message) { *error_message = "rows output pointer is null"; }
        return false;
    }

    std::string output;
    if (!run_mysql_command(sql, true, &output, error_message)) { return false; }

    rows->clear();
    std::stringstream stream(output);
    std::string       line;
    while (std::getline(stream, line)) {
        if (line.empty()) { continue; }
        rows->push_back(split_tab_line(line));
    }
    return true;
}

bool MysqlDriver::get_table_metadata(
    const std::string& table_name,
    TableMetadata*     metadata,
    std::string*       error_message
) {
    if (!metadata) {
        if (error_message) { *error_message = "metadata output pointer is null"; }
        return false;
    }

    std::vector<std::vector<std::string>> rows;
    {
        const std::string sql =
            "SELECT COLUMN_NAME, COLUMN_TYPE, COLUMN_DEFAULT, (COLUMN_DEFAULT IS NULL), IS_NULLABLE, EXTRA, "
            "CHARACTER_MAXIMUM_LENGTH, NUMERIC_PRECISION, NUMERIC_SCALE "
            "FROM information_schema.COLUMNS "
            "WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = '" + table_name + "' "
            "ORDER BY ORDINAL_POSITION";
        if (!query(sql, &rows, error_message)) { return false; }
    }

    metadata->table_name = table_name;
    metadata->columns.clear();
    for (const auto& row : rows) {
        if (row.size() < 9) { continue; }
        ColumnMetadata column;
        column.name          = row[0];
        column.data_type     = row[1];
        const bool default_is_null = (row[3] == "1");
        column.default_value = default_is_null ? std::nullopt : std::optional<std::string>(row[2]);
        column.nullable      = (row[4] == "YES");
        column.auto_increment = (row[5].find("auto_increment") != std::string::npos);
        column.unsigned_number = (row[1].find("unsigned") != std::string::npos);
        if (!row[6].empty()) {
            try {
                column.character_length = std::stoi(row[6]);
            } catch (...) {
            }
        }
        if (!row[7].empty()) {
            try {
                column.numeric_precision = std::stoi(row[7]);
            } catch (...) {
            }
        }
        if (!row[8].empty()) {
            try {
                column.numeric_scale = std::stoi(row[8]);
            } catch (...) {
            }
        }
        metadata->columns.push_back(std::move(column));
    }

    rows.clear();
    {
        const std::string sql =
            "SELECT COLUMN_NAME, REFERENCED_TABLE_NAME, REFERENCED_COLUMN_NAME "
            "FROM information_schema.KEY_COLUMN_USAGE "
            "WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = '" + table_name + "' "
            "AND REFERENCED_TABLE_NAME IS NOT NULL";
        if (!query(sql, &rows, error_message)) { return false; }
    }
    metadata->foreign_keys.clear();
    for (const auto& row : rows) {
        if (row.size() < 3) { continue; }
        metadata->foreign_keys.push_back(
            ForeignKeyMetadata{.column_name = row[0], .referenced_table = row[1], .referenced_column = row[2]}
        );
    }

    rows.clear();
    {
        const std::string sql =
            "SELECT INDEX_NAME, NON_UNIQUE, COLUMN_NAME "
            "FROM information_schema.STATISTICS "
            "WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = '" + table_name + "' "
            "ORDER BY INDEX_NAME, SEQ_IN_INDEX";
        if (!query(sql, &rows, error_message)) { return false; }
    }
    metadata->indexes.clear();
    for (const auto& row : rows) {
        if (row.size() < 3) { continue; }
        const std::string& index_name = row[0];
        auto               it = std::find_if(metadata->indexes.begin(), metadata->indexes.end(), [&](const IndexMetadata& index) {
            return index.name == index_name;
        });
        if (it == metadata->indexes.end()) {
            IndexMetadata index;
            index.name = index_name;
            index.unique = (row[1] == "0");
            index.columns.push_back(row[2]);
            metadata->indexes.push_back(std::move(index));
        } else {
            it->columns.push_back(row[2]);
        }
    }

    rows.clear();
    {
        const std::string sql =
            "SELECT TRIGGER_NAME, EVENT_MANIPULATION "
            "FROM information_schema.TRIGGERS "
            "WHERE TRIGGER_SCHEMA = DATABASE() AND EVENT_OBJECT_TABLE = '" + table_name + "'";
        if (!query(sql, &rows, error_message)) { return false; }
    }
    metadata->triggers.clear();
    for (const auto& row : rows) {
        if (row.size() < 2) { continue; }
        metadata->triggers.push_back(TriggerMetadata{.name = row[0], .event = row[1]});
    }

    rows.clear();
    {
        const std::string sql =
            "SELECT PARTITION_NAME FROM information_schema.PARTITIONS "
            "WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = '" + table_name + "' AND PARTITION_NAME IS NOT NULL LIMIT 1";
        if (!query(sql, &rows, error_message)) { return false; }
    }
    metadata->partitioned = !rows.empty();

    return true;
}

bool MysqlDriver::supports_load_mode() const {
    if (!connected_) { return false; }

    std::string output;
    std::string error;
    if (!run_mysql_command("SHOW VARIABLES LIKE 'local_infile'", true, &output, &error)) { return false; }

    std::transform(output.begin(), output.end(), output.begin(), [](const unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return output.find("\ton") != std::string::npos;
}

bool MysqlDriver::run_mysql_command(
    const std::string& sql,
    const bool         expect_rows,
    std::string*       stdout_text,
    std::string*       error_message
) const {
    if (!connected_) {
        if (error_message) { *error_message = "driver is not connected"; }
        return false;
    }

    const std::string mysql_binary = read_env_value("DATA_GENERATOR_MYSQL_BIN").value_or(kMysqlBinary);

    std::filesystem::path sql_file;
    if (!write_temp_sql_file(sql, &sql_file, error_message)) { return false; }

    std::ostringstream command;
    command << shell_escape(mysql_binary)
            << " --protocol=TCP"
            << " --user=" << shell_escape(connection_.username)
            << " --password=" << shell_escape(connection_.password)
            << " --host=" << shell_escape(connection_.host)
            << " --port=" << connection_.port
            << " --database=" << shell_escape(connection_.database)
            << " --default-character-set=utf8mb4"
            << " --local-infile=1"
            << " --raw --batch --skip-column-names";
    if (expect_rows) { command << " --silent"; }
    command << " < " << shell_escape(sql_file.string()) << " 2>&1";

#if defined(_WIN32)
    FILE* pipe = _popen(command.str().c_str(), "r");
#else
    FILE* pipe = popen(command.str().c_str(), "r");
#endif
    if (!pipe) {
        std::error_code ec;
        std::filesystem::remove(sql_file, ec);
        if (error_message) { *error_message = "failed to start mysql command"; }
        return false;
    }

    std::array<char, 4096> buffer{};
    std::string            output;
    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr) {
        output += buffer.data();
    }

#if defined(_WIN32)
    const int rc = _pclose(pipe);
#else
    const int rc = pclose(pipe);
#endif
    std::error_code remove_ec;
    std::filesystem::remove(sql_file, remove_ec);
    if (stdout_text) { *stdout_text = output; }

    if (rc != 0) {
        if (error_message) {
            *error_message = trim_line(output);
            if (error_message->empty()) { *error_message = "mysql command failed"; }
        }
        return false;
    }

    return true;
}

std::vector<std::string> MysqlDriver::split_tab_line(const std::string& line) {
    std::vector<std::string> parts;
    std::string              current;
    for (const char ch : line) {
        if (ch == '\t') {
            parts.push_back(current);
            current.clear();
        } else {
            current.push_back(ch);
        }
    }
    parts.push_back(std::move(current));
    return parts;
}

}  // namespace data_generator::database
