// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file db_url_parser.cpp

#include "output/database/db_url_parser.h"

#include <cstring>

namespace data_generator::database {

namespace {

constexpr const char* kOdbcPrefix   = "odbc://";
constexpr const char* kSqlitePrefix = "sqlite://";

bool parse_odbc_connection(const std::string& connection, DbUrl* parsed, std::string* error_message) {
    if (connection.rfind(kOdbcPrefix, 0) != 0) { return false; }

    const std::string odbc_connection_string = connection.substr(std::strlen(kOdbcPrefix));
    if (odbc_connection_string.empty()) {
        if (error_message) { *error_message = "odbc connection string is empty"; }
        return false;
    }

    parsed->type                   = DbType::Odbc;
    parsed->raw                    = connection;
    parsed->odbc_connection_string = odbc_connection_string;
    parsed->database.clear();
    return true;
}

bool parse_sqlite_connection(const std::string& connection, DbUrl* parsed, std::string* error_message) {
    if (connection.rfind(kSqlitePrefix, 0) != 0) { return false; }

    const std::string sqlite_target = connection.substr(std::strlen(kSqlitePrefix));
    if (sqlite_target.empty()) {
        if (error_message) {
            *error_message = "sqlite connection must use sqlite:///path/to/file.db or sqlite://:memory:";
        }
        return false;
    }
    if (sqlite_target != ":memory:" && sqlite_target.front() != '/') {
        if (error_message) {
            *error_message = "sqlite connection must use sqlite:///path/to/file.db or sqlite://:memory:";
        }
        return false;
    }

    parsed->type = DbType::Sqlite;
    parsed->raw  = connection;
    parsed->odbc_connection_string.clear();
    parsed->database = sqlite_target;
    return true;
}

}  // namespace

bool parse_db_connection(const std::string& connection, DbUrl* parsed, std::string* error_message) {
    if (!parsed) {
        if (error_message) { *error_message = "output pointer is null"; }
        return false;
    }

    DbUrl result;
    if (parse_sqlite_connection(connection, &result, error_message)) {
        *parsed = std::move(result);
        return true;
    }
    if (parse_odbc_connection(connection, &result, error_message)) {
        *parsed = std::move(result);
        return true;
    }

    if (error_message) {
        *error_message =
            "unsupported connection format, expected odbc://<connection_string> or sqlite:///path/to/file.db "
            "or sqlite://:memory:";
    }
    return false;
}

}  // namespace data_generator::database
