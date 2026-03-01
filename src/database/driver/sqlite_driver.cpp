// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.

#include "database/driver/sqlite_driver.h"

namespace data_generator::database {

DbType SqliteDriver::type() const {
    return DbType::Sqlite;
}

bool SqliteDriver::connect(const DbUrl& /*url*/, std::string* error_message) {
    return not_implemented(error_message);
}

void SqliteDriver::disconnect() {}

bool SqliteDriver::test_connection(std::string* error_message) {
    return not_implemented(error_message);
}

bool SqliteDriver::execute(const std::string& /*sql*/, std::string* error_message) {
    return not_implemented(error_message);
}

bool SqliteDriver::query(
    const std::string& /*sql*/,
    std::vector<std::vector<std::string>>* /*rows*/,
    std::string* error_message
) {
    return not_implemented(error_message);
}

bool SqliteDriver::get_table_metadata(
    const std::string& /*table_name*/,
    TableMetadata* /*metadata*/,
    std::string* error_message
) {
    return not_implemented(error_message);
}

bool SqliteDriver::supports_load_mode() const {
    return false;
}

bool SqliteDriver::not_implemented(std::string* error_message) {
    if (error_message) { *error_message = "SQLite driver is not implemented yet"; }
    return false;
}

}  // namespace data_generator::database
