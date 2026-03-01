// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

#include "database/db_metadata.h"

#include <algorithm>
#include <cctype>

namespace data_generator::database {

namespace {

std::string to_lower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return text;
}

bool contains(const std::string& text, const std::string& pattern) {
    return text.find(pattern) != std::string::npos;
}

}  // namespace

std::string db_type_to_string(const DbType type) {
    switch (type) {
    case DbType::Mysql     : return "mysql";
    case DbType::Postgresql: return "postgresql";
    case DbType::Sqlite    : return "sqlite";
    case DbType::Oracle    : return "oracle";
    case DbType::Unknown   : return "unknown";
    }
    return "unknown";
}

ColumnTypeFamily classify_column_type(const ColumnMetadata& column) {
    const std::string type = to_lower(column.data_type);
    if (type == "tinyint(1)" || type == "boolean" || type == "bool") { return ColumnTypeFamily::Boolean; }
    if (contains(type, "int")) { return ColumnTypeFamily::Integer; }
    if (contains(type, "decimal") || contains(type, "numeric") || contains(type, "float") || contains(type, "double")) {
        return ColumnTypeFamily::Decimal;
    }
    if (contains(type, "datetime") || contains(type, "timestamp")) { return ColumnTypeFamily::DateTime; }
    if (type == "date") { return ColumnTypeFamily::Date; }
    if (type == "time") { return ColumnTypeFamily::Time; }
    if (contains(type, "enum")) { return ColumnTypeFamily::Enum; }
    if (contains(type, "blob") || contains(type, "binary")) { return ColumnTypeFamily::Binary; }
    if (contains(type, "char") || contains(type, "text") || contains(type, "json")) { return ColumnTypeFamily::String; }
    return ColumnTypeFamily::Unknown;
}

}  // namespace data_generator::database
