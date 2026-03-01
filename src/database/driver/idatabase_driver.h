// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

#ifndef DATA_GENERATOR_DATABASE_DRIVER_IDATABASE_DRIVER_H
#define DATA_GENERATOR_DATABASE_DRIVER_IDATABASE_DRIVER_H

#include <memory>
#include <string>
#include <vector>

#include "database/db_metadata.h"

namespace data_generator::database {

class IDatabaseDriver {
public:
    virtual ~IDatabaseDriver() = default;

    virtual DbType type() const = 0;

    virtual bool connect(const DbUrl& url, std::string* error_message) = 0;
    virtual void disconnect() = 0;

    virtual bool test_connection(std::string* error_message) = 0;

    virtual bool execute(const std::string& sql, std::string* error_message) = 0;

    virtual bool query(
        const std::string&                    sql,
        std::vector<std::vector<std::string>>* rows,
        std::string*                          error_message
    ) = 0;

    virtual bool get_table_metadata(
        const std::string& table_name,
        TableMetadata*     metadata,
        std::string*       error_message
    ) = 0;

    virtual bool supports_load_mode() const = 0;
};

std::unique_ptr<IDatabaseDriver> make_database_driver(DbType type);

}  // namespace data_generator::database

#endif
