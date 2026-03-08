// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file idatabase_driver.cpp

#include "database/driver/idatabase_driver.h"

#include "database/driver/odbc_driver.h"

namespace data_generator::database {

std::unique_ptr<IDatabaseDriver> make_database_driver(const DbType type) {
    switch (type) {
    case DbType::Mysql:
    case DbType::Postgresql:
    case DbType::Oracle:
        return std::make_unique<OdbcDriver>(type);
    case DbType::Unknown   : return nullptr;
    }
    return nullptr;
}

}  // namespace data_generator::database
