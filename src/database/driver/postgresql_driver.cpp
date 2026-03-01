// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.

#include "database/driver/postgresql_driver.h"

namespace data_generator::database {

DbType PostgresqlDriver::type() const {
    return DbType::Postgresql;
}

bool PostgresqlDriver::connect(const DbUrl& /*url*/, std::string* error_message) {
    return not_implemented(error_message);
}

void PostgresqlDriver::disconnect() {}

bool PostgresqlDriver::test_connection(std::string* error_message) {
    return not_implemented(error_message);
}

bool PostgresqlDriver::execute(const std::string& /*sql*/, std::string* error_message) {
    return not_implemented(error_message);
}

bool PostgresqlDriver::query(
    const std::string& /*sql*/,
    std::vector<std::vector<std::string>>* /*rows*/,
    std::string* error_message
) {
    return not_implemented(error_message);
}

bool PostgresqlDriver::get_table_metadata(
    const std::string& /*table_name*/,
    TableMetadata* /*metadata*/,
    std::string* error_message
) {
    return not_implemented(error_message);
}

bool PostgresqlDriver::supports_load_mode() const {
    return false;
}

bool PostgresqlDriver::not_implemented(std::string* error_message) {
    if (error_message) { *error_message = "PostgreSQL driver is not implemented yet"; }
    return false;
}

}  // namespace data_generator::database
