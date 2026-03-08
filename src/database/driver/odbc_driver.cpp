// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file odbc_driver.cpp

#include "database/driver/odbc_driver.h"

#include <algorithm>
#include <cctype>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>

namespace data_generator::database {

namespace {

constexpr std::size_t kOdbcTextBufferSize = 4096;

std::string to_lower_ascii(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](const unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

std::string escape_sql_literal(const std::string& value) {
    std::string escaped;
    escaped.reserve(value.size() + 8);
    for (const char ch : value) {
        escaped.push_back(ch);
        if (ch == '\'') { escaped.push_back('\''); }
    }
    return escaped;
}

std::string parse_odbc_diagnostics(const SQLSMALLINT handle_type, const SQLHANDLE handle) {
    std::ostringstream message;
    SQLSMALLINT        rec_number = 1;

    while (true) {
        SQLCHAR     state[6] = {0};
        SQLINTEGER  native_error = 0;
        SQLCHAR     text[kOdbcTextBufferSize] = {0};
        SQLSMALLINT text_length = 0;
        const SQLRETURN rc = SQLGetDiagRec(
            handle_type,
            handle,
            rec_number,
            state,
            &native_error,
            text,
            static_cast<SQLSMALLINT>(sizeof(text)),
            &text_length
        );
        if (rc == SQL_NO_DATA) { break; }
        if (!SQL_SUCCEEDED(rc)) {
            if (rec_number == 1) { return "ODBC operation failed (diagnostics unavailable)"; }
            break;
        }

        if (rec_number > 1) { message << " | "; }
        message << "[" << reinterpret_cast<const char*>(state) << "] "
                << reinterpret_cast<const char*>(text);
        ++rec_number;
    }

    const std::string result = message.str();
    return result.empty() ? "ODBC operation failed" : result;
}

std::pair<std::string, std::string> split_schema_table(const std::string& table_name) {
    const std::size_t dot = table_name.find('.');
    if (dot == std::string::npos) { return {"", table_name}; }
    return {table_name.substr(0, dot), table_name.substr(dot + 1)};
}

std::optional<int> try_parse_optional_int(const std::string& value) {
    if (value.empty()) { return std::nullopt; }
    try {
        return std::stoi(value);
    } catch (...) {
        return std::nullopt;
    }
}

void load_index_metadata(
    const std::vector<std::vector<std::string>>& rows,
    TableMetadata* metadata,
    const std::unordered_map<std::string, std::size_t>& index_name_to_pos
) {
    if (!metadata) { return; }

    std::unordered_map<std::string, std::size_t> name_to_index = index_name_to_pos;
    for (const auto& row : rows) {
        if (row.size() < 3) { continue; }

        const std::string& index_name = row[0];
        auto               it = name_to_index.find(index_name);
        if (it == name_to_index.end()) {
            IndexMetadata index;
            index.name = index_name;
            index.unique = (row[1] == "0");
            index.columns.push_back(row[2]);
            metadata->indexes.push_back(std::move(index));
            name_to_index.emplace(index_name, metadata->indexes.size() - 1);
        } else {
            metadata->indexes[it->second].columns.push_back(row[2]);
        }
    }
}

}  // namespace

OdbcDriver::OdbcDriver(const DbType db_type) : db_type_(db_type) {}

OdbcDriver::~OdbcDriver() {
    disconnect();
}

DbType OdbcDriver::type() const {
    return db_type_;
}

bool OdbcDriver::connect(const DbUrl& url, std::string* error_message) {
    disconnect();

    if (url.odbc_connection_string.empty()) {
        if (error_message) { *error_message = "ODBC connection string is empty"; }
        return false;
    }

    connection_ = url;

    SQLRETURN rc = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &env_);
    if (!SQL_SUCCEEDED(rc)) {
        if (error_message) { *error_message = "failed to allocate ODBC environment"; }
        return false;
    }

    rc = SQLSetEnvAttr(env_, SQL_ATTR_ODBC_VERSION, reinterpret_cast<SQLPOINTER>(SQL_OV_ODBC3), 0);
    if (!SQL_SUCCEEDED(rc)) {
        if (error_message) { *error_message = parse_odbc_diagnostics(SQL_HANDLE_ENV, env_); }
        disconnect();
        return false;
    }

    rc = SQLAllocHandle(SQL_HANDLE_DBC, env_, &dbc_);
    if (!SQL_SUCCEEDED(rc)) {
        if (error_message) { *error_message = parse_odbc_diagnostics(SQL_HANDLE_ENV, env_); }
        disconnect();
        return false;
    }

    SQLCHAR out_buffer[1024] = {0};
    SQLSMALLINT out_length = 0;
    rc = SQLDriverConnect(
        dbc_,
        nullptr,
        reinterpret_cast<SQLCHAR*>(const_cast<char*>(connection_.odbc_connection_string.c_str())),
        SQL_NTS,
        out_buffer,
        static_cast<SQLSMALLINT>(sizeof(out_buffer)),
        &out_length,
        SQL_DRIVER_NOPROMPT
    );
    if (!SQL_SUCCEEDED(rc)) {
        if (error_message) { *error_message = parse_odbc_diagnostics(SQL_HANDLE_DBC, dbc_); }
        disconnect();
        return false;
    }

    connected_ = true;
    return true;
}

void OdbcDriver::disconnect() {
    if (dbc_ != SQL_NULL_HDBC) {
        if (connected_) { (void)SQLDisconnect(dbc_); }
        (void)SQLFreeHandle(SQL_HANDLE_DBC, dbc_);
        dbc_ = SQL_NULL_HDBC;
    }

    if (env_ != SQL_NULL_HENV) {
        (void)SQLFreeHandle(SQL_HANDLE_ENV, env_);
        env_ = SQL_NULL_HENV;
    }

    connected_ = false;
}

bool OdbcDriver::test_connection(std::string* error_message) {
    if (!connected_) {
        if (error_message) { *error_message = "driver is not connected"; }
        return false;
    }

    const std::string sql = (db_type_ == DbType::Oracle) ? "SELECT 1 FROM DUAL" : "SELECT 1";
    std::vector<std::vector<std::string>> rows;
    if (!run_query(sql, &rows, error_message)) { return false; }
    return !rows.empty();
}

bool OdbcDriver::execute(const std::string& sql, std::string* error_message) {
    if (!connected_) {
        if (error_message) { *error_message = "driver is not connected"; }
        return false;
    }

    SQLHSTMT stmt = SQL_NULL_HSTMT;
    const SQLRETURN alloc_rc = SQLAllocHandle(SQL_HANDLE_STMT, dbc_, &stmt);
    if (!SQL_SUCCEEDED(alloc_rc)) {
        if (error_message) { *error_message = parse_odbc_diagnostics(SQL_HANDLE_DBC, dbc_); }
        return false;
    }

    const bool ok = execute_statement(stmt, sql, error_message);
    (void)SQLFreeHandle(SQL_HANDLE_STMT, stmt);
    return ok;
}

bool OdbcDriver::query(
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

bool OdbcDriver::get_table_metadata(
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

    metadata->table_name = table_name;
    metadata->columns.clear();
    metadata->foreign_keys.clear();
    metadata->indexes.clear();
    metadata->triggers.clear();
    metadata->partitioned = false;

    if (db_type_ == DbType::Mysql) { return load_mysql_metadata(table_name, metadata, error_message); }
    if (db_type_ == DbType::Postgresql) { return load_postgresql_metadata(table_name, metadata, error_message); }
    if (db_type_ == DbType::Oracle) { return load_oracle_metadata(table_name, metadata, error_message); }

    if (error_message) { *error_message = "unsupported database type"; }
    return false;
}

bool OdbcDriver::supports_load_mode() const {
    return db_type_ == DbType::Mysql || db_type_ == DbType::Postgresql;
}

bool OdbcDriver::execute_statement(SQLHSTMT stmt, const std::string& sql, std::string* error_message) const {
    const SQLRETURN exec_rc = SQLExecDirect(stmt, reinterpret_cast<SQLCHAR*>(const_cast<char*>(sql.c_str())), SQL_NTS);
    if (!SQL_SUCCEEDED(exec_rc) && exec_rc != SQL_NO_DATA) {
        if (error_message) { *error_message = parse_odbc_diagnostics(SQL_HANDLE_STMT, stmt); }
        return false;
    }
    return true;
}

bool OdbcDriver::run_query(
    const std::string&                     sql,
    std::vector<std::vector<std::string>>* rows,
    std::string*                           error_message
) const {
    rows->clear();

    SQLHSTMT stmt = SQL_NULL_HSTMT;
    const SQLRETURN alloc_rc = SQLAllocHandle(SQL_HANDLE_STMT, dbc_, &stmt);
    if (!SQL_SUCCEEDED(alloc_rc)) {
        if (error_message) { *error_message = parse_odbc_diagnostics(SQL_HANDLE_DBC, dbc_); }
        return false;
    }

    if (!execute_statement(stmt, sql, error_message)) {
        (void)SQLFreeHandle(SQL_HANDLE_STMT, stmt);
        return false;
    }

    SQLSMALLINT col_count = 0;
    if (!SQL_SUCCEEDED(SQLNumResultCols(stmt, &col_count))) {
        if (error_message) { *error_message = parse_odbc_diagnostics(SQL_HANDLE_STMT, stmt); }
        (void)SQLFreeHandle(SQL_HANDLE_STMT, stmt);
        return false;
    }

    while (true) {
        const SQLRETURN fetch_rc = SQLFetch(stmt);
        if (fetch_rc == SQL_NO_DATA) { break; }
        if (!SQL_SUCCEEDED(fetch_rc)) {
            if (error_message) { *error_message = parse_odbc_diagnostics(SQL_HANDLE_STMT, stmt); }
            (void)SQLFreeHandle(SQL_HANDLE_STMT, stmt);
            return false;
        }

        std::vector<std::string> row;
        row.reserve(static_cast<std::size_t>(col_count));

        for (SQLUSMALLINT col = 1; col <= static_cast<SQLUSMALLINT>(col_count); ++col) {
            std::string value;
            while (true) {
                SQLCHAR     buffer[kOdbcTextBufferSize] = {0};
                SQLLEN      indicator = 0;
                const SQLRETURN data_rc = SQLGetData(
                    stmt,
                    col,
                    SQL_C_CHAR,
                    buffer,
                    sizeof(buffer),
                    &indicator
                );

                if (indicator == SQL_NULL_DATA) {
                    value.clear();
                    break;
                }

                if (!SQL_SUCCEEDED(data_rc) && data_rc != SQL_SUCCESS_WITH_INFO) {
                    if (error_message) { *error_message = parse_odbc_diagnostics(SQL_HANDLE_STMT, stmt); }
                    (void)SQLFreeHandle(SQL_HANDLE_STMT, stmt);
                    return false;
                }

                value += reinterpret_cast<const char*>(buffer);
                if (data_rc == SQL_SUCCESS) { break; }
            }
            row.push_back(std::move(value));
        }

        rows->push_back(std::move(row));
    }

    (void)SQLFreeHandle(SQL_HANDLE_STMT, stmt);
    return true;
}

bool OdbcDriver::load_mysql_metadata(
    const std::string& table_name,
    TableMetadata*     metadata,
    std::string*       error_message
) {
    std::vector<std::vector<std::string>> rows;

    const std::string escaped_table_name = escape_sql_literal(table_name);

    const std::string column_sql =
        "SELECT COLUMN_NAME, COLUMN_TYPE, COLUMN_DEFAULT, (COLUMN_DEFAULT IS NULL), IS_NULLABLE, EXTRA, "
        "CHARACTER_MAXIMUM_LENGTH, NUMERIC_PRECISION, NUMERIC_SCALE "
        "FROM information_schema.COLUMNS "
        "WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = '" + escaped_table_name + "' "
        "ORDER BY ORDINAL_POSITION";
    if (!run_query(column_sql, &rows, error_message)) { return false; }

    for (const auto& row : rows) {
        if (row.size() < 9) { continue; }

        ColumnMetadata column;
        column.name = row[0];
        column.data_type = row[1];
        const bool default_is_null = (row[3] == "1");
        column.default_value = default_is_null ? std::nullopt : std::optional<std::string>(row[2]);
        column.nullable = (row[4] == "YES");
        column.auto_increment = (to_lower_ascii(row[5]).find("auto_increment") != std::string::npos);
        column.unsigned_number = (to_lower_ascii(row[1]).find("unsigned") != std::string::npos);
        column.character_length = try_parse_optional_int(row[6]);
        column.numeric_precision = try_parse_optional_int(row[7]);
        column.numeric_scale = try_parse_optional_int(row[8]);

        metadata->columns.push_back(std::move(column));
    }

    rows.clear();
    const std::string fk_sql =
        "SELECT COLUMN_NAME, REFERENCED_TABLE_NAME, REFERENCED_COLUMN_NAME "
        "FROM information_schema.KEY_COLUMN_USAGE "
        "WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = '" + escaped_table_name + "' "
        "AND REFERENCED_TABLE_NAME IS NOT NULL";
    if (!run_query(fk_sql, &rows, error_message)) { return false; }

    for (const auto& row : rows) {
        if (row.size() < 3) { continue; }
        metadata->foreign_keys.push_back(ForeignKeyMetadata{
            .column_name = row[0],
            .referenced_table = row[1],
            .referenced_column = row[2],
        });
    }

    rows.clear();
    const std::string index_sql =
        "SELECT INDEX_NAME, NON_UNIQUE, COLUMN_NAME "
        "FROM information_schema.STATISTICS "
        "WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = '" + escaped_table_name + "' "
        "ORDER BY INDEX_NAME, SEQ_IN_INDEX";
    if (!run_query(index_sql, &rows, error_message)) { return false; }

    const std::unordered_map<std::string, std::size_t> empty_map;
    load_index_metadata(rows, metadata, empty_map);

    rows.clear();
    const std::string trigger_sql =
        "SELECT TRIGGER_NAME, EVENT_MANIPULATION "
        "FROM information_schema.TRIGGERS "
        "WHERE TRIGGER_SCHEMA = DATABASE() AND EVENT_OBJECT_TABLE = '" + escaped_table_name + "'";
    if (!run_query(trigger_sql, &rows, error_message)) { return false; }

    for (const auto& row : rows) {
        if (row.size() < 2) { continue; }
        metadata->triggers.push_back(TriggerMetadata{.name = row[0], .event = row[1]});
    }

    rows.clear();
    const std::string partition_sql =
        "SELECT PARTITION_NAME FROM information_schema.PARTITIONS "
        "WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = '" + escaped_table_name +
        "' AND PARTITION_NAME IS NOT NULL LIMIT 1";
    if (!run_query(partition_sql, &rows, error_message)) { return false; }
    metadata->partitioned = !rows.empty();

    return true;
}

bool OdbcDriver::load_postgresql_metadata(
    const std::string& table_name,
    TableMetadata*     metadata,
    std::string*       error_message
) {
    std::vector<std::vector<std::string>> rows;
    const auto [schema_name, base_table_name] = split_schema_table(table_name);

    const std::string schema = escape_sql_literal(schema_name);
    const std::string table = escape_sql_literal(base_table_name);

    const std::string column_sql =
        "SELECT c.column_name, c.udt_name, c.column_default, (c.column_default IS NULL), "
        "(c.is_nullable = 'YES'), FALSE, c.character_maximum_length, c.numeric_precision, c.numeric_scale "
        "FROM information_schema.columns c "
        "WHERE c.table_schema = COALESCE(NULLIF('" + schema + "', ''), current_schema()) "
        "AND c.table_name = '" + table + "' "
        "ORDER BY c.ordinal_position";
    if (!run_query(column_sql, &rows, error_message)) { return false; }

    for (const auto& row : rows) {
        if (row.size() < 9) { continue; }

        ColumnMetadata column;
        column.name = row[0];
        column.data_type = row[1];
        const bool default_is_null = (row[3] == "1" || to_lower_ascii(row[3]) == "true");
        column.default_value = default_is_null ? std::nullopt : std::optional<std::string>(row[2]);
        column.nullable = (row[4] == "1" || to_lower_ascii(row[4]) == "true");
        column.auto_increment = false;
        column.unsigned_number = false;
        column.character_length = try_parse_optional_int(row[6]);
        column.numeric_precision = try_parse_optional_int(row[7]);
        column.numeric_scale = try_parse_optional_int(row[8]);

        metadata->columns.push_back(std::move(column));
    }

    rows.clear();
    const std::string fk_sql =
        "SELECT kcu.column_name, ccu.table_name, ccu.column_name "
        "FROM information_schema.table_constraints tc "
        "JOIN information_schema.key_column_usage kcu "
        "  ON tc.constraint_name = kcu.constraint_name AND tc.table_schema = kcu.table_schema "
        "JOIN information_schema.constraint_column_usage ccu "
        "  ON ccu.constraint_name = tc.constraint_name AND ccu.table_schema = tc.table_schema "
        "WHERE tc.constraint_type = 'FOREIGN KEY' "
        "AND tc.table_schema = COALESCE(NULLIF('" + schema + "', ''), current_schema()) "
        "AND tc.table_name = '" + table + "'";
    if (!run_query(fk_sql, &rows, error_message)) { return false; }

    for (const auto& row : rows) {
        if (row.size() < 3) { continue; }
        metadata->foreign_keys.push_back(ForeignKeyMetadata{
            .column_name = row[0],
            .referenced_table = row[1],
            .referenced_column = row[2],
        });
    }

    rows.clear();
    const std::string index_sql =
        "SELECT i.relname AS index_name, CASE WHEN ix.indisunique THEN 0 ELSE 1 END AS non_unique, "
        "a.attname AS column_name "
        "FROM pg_class t "
        "JOIN pg_namespace ns ON ns.oid = t.relnamespace "
        "JOIN pg_index ix ON t.oid = ix.indrelid "
        "JOIN pg_class i ON i.oid = ix.indexrelid "
        "JOIN pg_attribute a ON a.attrelid = t.oid AND a.attnum = ANY(ix.indkey) "
        "WHERE t.relkind = 'r' AND t.relname = '" + table + "' "
        "AND ns.nspname = COALESCE(NULLIF('" + schema + "', ''), current_schema()) "
        "ORDER BY i.relname, a.attnum";
    if (!run_query(index_sql, &rows, error_message)) { return false; }

    const std::unordered_map<std::string, std::size_t> empty_map;
    load_index_metadata(rows, metadata, empty_map);

    return true;
}

bool OdbcDriver::load_oracle_metadata(
    const std::string& table_name,
    TableMetadata*     metadata,
    std::string*       error_message
) {
    std::vector<std::vector<std::string>> rows;
    const auto [schema_name, base_table_name] = split_schema_table(table_name);
    const std::string table = escape_sql_literal(base_table_name);

    std::string column_sql;
    if (schema_name.empty()) {
        column_sql =
            "SELECT utc.column_name, utc.data_type, utc.data_default, "
            "CASE WHEN utc.data_default IS NULL THEN 1 ELSE 0 END, "
            "CASE WHEN utc.nullable = 'Y' THEN 1 ELSE 0 END, "
            "CASE WHEN utc.identity_column = 'YES' THEN 1 ELSE 0 END, "
            "utc.char_length, utc.data_precision, utc.data_scale "
            "FROM user_tab_columns utc "
            "WHERE utc.table_name = UPPER('" + table + "') "
            "ORDER BY utc.column_id";
    } else {
        const std::string schema = escape_sql_literal(schema_name);
        column_sql =
            "SELECT atc.column_name, atc.data_type, atc.data_default, "
            "CASE WHEN atc.data_default IS NULL THEN 1 ELSE 0 END, "
            "CASE WHEN atc.nullable = 'Y' THEN 1 ELSE 0 END, "
            "CASE WHEN atc.identity_column = 'YES' THEN 1 ELSE 0 END, "
            "atc.char_length, atc.data_precision, atc.data_scale "
            "FROM all_tab_columns atc "
            "WHERE atc.owner = UPPER('" + schema + "') "
            "AND atc.table_name = UPPER('" + table + "') "
            "ORDER BY atc.column_id";
    }

    if (!run_query(column_sql, &rows, error_message)) { return false; }

    for (const auto& row : rows) {
        if (row.size() < 9) { continue; }

        ColumnMetadata column;
        column.name = row[0];
        column.data_type = row[1];
        const bool default_is_null = (row[3] == "1");
        column.default_value = default_is_null ? std::nullopt : std::optional<std::string>(row[2]);
        column.nullable = (row[4] == "1");
        column.auto_increment = (row[5] == "1");
        column.unsigned_number = false;
        column.character_length = try_parse_optional_int(row[6]);
        column.numeric_precision = try_parse_optional_int(row[7]);
        column.numeric_scale = try_parse_optional_int(row[8]);

        metadata->columns.push_back(std::move(column));
    }

    rows.clear();
    const std::string index_sql =
        "SELECT ui.index_name, CASE WHEN ui.uniqueness = 'UNIQUE' THEN 0 ELSE 1 END AS non_unique, "
        "uic.column_name "
        "FROM user_indexes ui "
        "JOIN user_ind_columns uic ON ui.index_name = uic.index_name "
        "WHERE ui.table_name = UPPER('" + table + "') "
        "ORDER BY ui.index_name, uic.column_position";
    if (!run_query(index_sql, &rows, error_message)) { return false; }

    const std::unordered_map<std::string, std::size_t> empty_map;
    load_index_metadata(rows, metadata, empty_map);

    return true;
}

}  // namespace data_generator::database
