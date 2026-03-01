// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.

#ifndef DATA_GENERATOR_DATABASE_DRIVER_POSTGRESQL_DRIVER_H
#define DATA_GENERATOR_DATABASE_DRIVER_POSTGRESQL_DRIVER_H

#include "database/driver/idatabase_driver.h"

namespace data_generator::database {

class PostgresqlDriver final : public IDatabaseDriver {
public:
    DbType type() const override;
    bool connect(const DbUrl& url, std::string* error_message) override;
    void disconnect() override;
    bool test_connection(std::string* error_message) override;
    bool execute(const std::string& sql, std::string* error_message) override;
    bool query(const std::string& sql, std::vector<std::vector<std::string>>* rows, std::string* error_message)
        override;
    bool get_table_metadata(const std::string& table_name, TableMetadata* metadata, std::string* error_message)
        override;
    bool supports_load_mode() const override;

private:
    static bool not_implemented(std::string* error_message);
};

}  // namespace data_generator::database

#endif
