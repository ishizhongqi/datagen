// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file sql_writer.cpp

#include "output/file/sql_writer.h"

#include <algorithm>
#include <cctype>
#include <optional>

namespace data_generator::output::file {

namespace {

std::optional<bool> parse_boolean_token(const std::string& raw) {
    std::string normalized = raw;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    if (normalized == "true" || normalized == "1") { return true; }
    if (normalized == "false" || normalized == "0") { return false; }
    return std::nullopt;
}

}  // namespace

std::string sql_escape(const std::string& value) {
    std::string escaped;
    escaped.reserve(value.size());
    for (const char ch : value) {
        if (ch == '\'') { escaped.push_back('\''); }
        escaped.push_back(ch);
    }
    return escaped;
}

void write_sql_row(
    const std::vector<std::string>& columns,
    const std::vector<bool>&        boolean_columns,
    const engine::Row&              row,
    const std::string&              table_name,
    std::ostream&                   out
) {
    out << "INSERT INTO " << table_name << " (";
    for (std::string::size_type i = 0; i < columns.size(); ++i) {
        if (i > 0) { out << ", "; }
        out << columns[i];
    }
    out << ") VALUES (";
    for (std::string::size_type i = 0; i < row.size(); ++i) {
        if (i > 0) { out << ", "; }
        if (!row[i].has_value()) {
            out << "NULL";
        } else {
            if (i < boolean_columns.size() && boolean_columns[i]) {
                if (const auto bool_value = parse_boolean_token(*row[i]); bool_value.has_value()) {
                    out << (*bool_value ? "TRUE" : "FALSE");
                    continue;
                }
            }
            out << "'" << sql_escape(*row[i]) << "'";
        }
    }
    out << ");\n";
}

void write_sql(
    const std::vector<std::string>& columns,
    const std::vector<bool>&        boolean_columns,
    const std::vector<engine::Row>& rows,
    const std::string&              table_name,
    const bool                      create_table,
    std::ostream&                   out
) {
    if (create_table) {
        out << "CREATE TABLE IF NOT EXISTS " << table_name << " (";
        for (std::string::size_type i = 0; i < columns.size(); ++i) {
            if (i > 0) { out << ", "; }
            out << columns[i] << (i < boolean_columns.size() && boolean_columns[i] ? " BOOLEAN" : " TEXT");
        }
        out << ");\n";
    }
    for (const auto& row : rows) { write_sql_row(columns, boolean_columns, row, table_name, out); }
}

}  // namespace data_generator::output::file
