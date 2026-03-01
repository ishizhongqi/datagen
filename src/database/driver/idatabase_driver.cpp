// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.

#include "database/driver/idatabase_driver.h"

#include "database/driver/mysql_driver.h"
#include "database/driver/oracle_driver.h"
#include "database/driver/postgresql_driver.h"
#include "database/driver/sqlite_driver.h"

namespace data_generator::database {

std::unique_ptr<IDatabaseDriver> make_database_driver(const DbType type) {
    switch (type) {
    case DbType::Mysql     : return std::make_unique<MysqlDriver>();
    case DbType::Postgresql: return std::make_unique<PostgresqlDriver>();
    case DbType::Sqlite    : return std::make_unique<SqliteDriver>();
    case DbType::Oracle    : return std::make_unique<OracleDriver>();
    case DbType::Unknown   : return nullptr;
    }
    return nullptr;
}

}  // namespace data_generator::database
