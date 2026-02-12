// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file serialization.cpp

#include "core/serialization.h"

#include <nlohmann/json.hpp>

namespace data_generator::core {

std::string csv_escape(const std::string& value) {
    if (value.find_first_of(",\"\n\r") == std::string::npos) { return value; }
    std::string escaped;
    escaped.reserve(value.size() + 2);
    escaped.push_back('"');
    for (const char ch : value) {
        if (ch == '"') { escaped.push_back('"'); }
        escaped.push_back(ch);
    }
    escaped.push_back('"');
    return escaped;
}

std::string sql_escape(const std::string& value) {
    std::string escaped;
    escaped.reserve(value.size());
    for (const char ch : value) {
        if (ch == '\'') { escaped.push_back('\''); }
        escaped.push_back(ch);
    }
    return escaped;
}

void write_csv(const std::vector<std::string>& columns, const std::vector<Row>& rows, std::ostream& out) {
    for (size_t i = 0; i < columns.size(); ++i) {
        if (i > 0) { out << ","; }
        out << csv_escape(columns[i]);
    }
    out << "\n";

    for (const auto& row : rows) {
        for (size_t i = 0; i < row.size(); ++i) {
            if (i > 0) { out << ","; }
            out << csv_escape(row[i].value_or(""));
        }
        out << "\n";
    }
}

void write_json(const std::vector<std::string>& columns, const std::vector<Row>& rows, std::ostream& out) {
    nlohmann::json result = nlohmann::json::array();
    result.get_ref<nlohmann::json::array_t&>().reserve(rows.size());

    for (const auto& row : rows) {
        nlohmann::json entry = nlohmann::json::object();
        for (size_t i = 0; i < columns.size(); ++i) {
            if (i >= row.size() || !row[i].has_value()) {
                entry[columns[i]] = nullptr;
            } else {
                entry[columns[i]] = *row[i];
            }
        }
        result.push_back(std::move(entry));
    }

    out << result.dump(2) << "\n";
}

void write_sql(
    const std::vector<std::string>& columns,
    const std::vector<Row>&         rows,
    const std::string&              table_name,
    const bool                      include_create_table,
    std::ostream&                   out
) {
    if (include_create_table) {
        out << "CREATE TABLE " << table_name << " (\n";
        for (size_t i = 0; i < columns.size(); ++i) {
            out << "  " << columns[i] << " TEXT";
            if (i + 1 < columns.size()) { out << ","; }
            out << "\n";
        }
        out << ");\n";
    }

    for (const auto& row : rows) {
        out << "INSERT INTO " << table_name << " (";
        for (size_t i = 0; i < columns.size(); ++i) {
            if (i > 0) { out << ", "; }
            out << columns[i];
        }
        out << ") VALUES (";
        for (size_t i = 0; i < row.size(); ++i) {
            if (i > 0) { out << ", "; }
            if (!row[i].has_value()) {
                out << "NULL";
            } else {
                out << "'" << sql_escape(*row[i]) << "'";
            }
        }
        out << ");\n";
    }
}

}  // namespace data_generator::core
