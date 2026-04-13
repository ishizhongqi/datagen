// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file db_metadata.h

#ifndef DATAGEN_DB_METADATA_H
#define DATAGEN_DB_METADATA_H

#include <optional>
#include <string>
#include <vector>

namespace datagen::database {

enum class DbType {
    Odbc,
    Mysql,
    Postgresql,
    Oracle,
    Sqlite,
    Unknown,
};

enum class ColumnTypeFamily {
    Integer,
    Decimal,
    Boolean,
    Date,
    Time,
    DateTime,
    Enum,
    String,
    Binary,
    Unknown,
};

struct DbUrl {
    DbType      type = DbType::Unknown;
    std::string raw;
    std::string odbc_connection_string;
    std::string username;
    std::string password;
    std::string host;
    int         port = 0;
    std::string database;
};

struct ColumnMetadata {
    std::string                name;
    std::string                data_type;
    std::optional<std::string> default_value;
    bool                       auto_increment  = false;
    bool                       unsigned_number = false;
    std::optional<int>         character_length;
    std::optional<int>         numeric_precision;
    std::optional<int>         numeric_scale;
    std::vector<std::string>   enum_values;
};

struct ForeignKeyMetadata {
    std::string column_name;
    std::string referenced_table;
    std::string referenced_column;
};

struct IndexMetadata {
    std::string              name;
    bool                     unique = false;
    std::vector<std::string> columns;
};

struct TriggerMetadata {
    std::string name;
    std::string event;
};

struct TableMetadata {
    std::string                     table_name;
    std::vector<ColumnMetadata>     columns;
    std::vector<ForeignKeyMetadata> foreign_keys;
    std::vector<IndexMetadata>      indexes;
    std::vector<TriggerMetadata>    triggers;
    bool                            partitioned = false;
};

std::string db_type_to_string(DbType type);

ColumnTypeFamily classify_column_type(const ColumnMetadata& column);

}  // namespace datagen::database

#endif
