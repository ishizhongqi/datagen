// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.

#include "database/driver/oracle_driver.h"

namespace data_generator::database {

DbType OracleDriver::type() const {
    return DbType::Oracle;
}

bool OracleDriver::connect(const DbUrl& /*url*/, std::string* error_message) {
    return not_implemented(error_message);
}

void OracleDriver::disconnect() {}

bool OracleDriver::test_connection(std::string* error_message) {
    return not_implemented(error_message);
}

bool OracleDriver::execute(const std::string& /*sql*/, std::string* error_message) {
    return not_implemented(error_message);
}

bool OracleDriver::query(
    const std::string& /*sql*/,
    std::vector<std::vector<std::string>>* /*rows*/,
    std::string* error_message
) {
    return not_implemented(error_message);
}

bool OracleDriver::get_table_metadata(
    const std::string& /*table_name*/,
    TableMetadata* /*metadata*/,
    std::string* error_message
) {
    return not_implemented(error_message);
}

bool OracleDriver::supports_load_mode() const {
    return false;
}

bool OracleDriver::not_implemented(std::string* error_message) {
    if (error_message) { *error_message = "Oracle driver is not implemented yet"; }
    return false;
}

}  // namespace data_generator::database
