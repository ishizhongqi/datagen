// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file idatabase_driver.cpp

#include "output/database/drivers/idatabase_driver.h"

#include "output/database/drivers/odbc_driver.h"
#include "output/database/drivers/sqlite_driver.h"

namespace data_generator::database {

std::unique_ptr<IDatabaseDriver> make_database_driver(const DbType type) {
    switch (type) {
    case DbType::Mysql:
    case DbType::Postgresql:
    case DbType::Oracle:
        return std::make_unique<OdbcDriver>(type);
    case DbType::Sqlite:
        return std::make_unique<SqliteDriver>();
    default:
        return nullptr;
    }
}

}  // namespace data_generator::database
