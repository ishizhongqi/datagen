// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file sqlite_driver.h

#ifndef DATA_GENERATOR_SQLITE_DRIVER_H
#define DATA_GENERATOR_SQLITE_DRIVER_H

#include <string>
#include <vector>

#include <sqlite3.h>

#include "output/database/drivers/idatabase_driver.h"

namespace data_generator::database {

class SqliteDriver final : public IDatabaseDriver {
public:
    SqliteDriver() = default;
    ~SqliteDriver() override;

    [[nodiscard]] DbType type() const override;
    [[nodiscard]] std::string dbms_name() const override;
    [[nodiscard]] std::string dbms_version() const override;

    bool connect(const DbUrl& url, std::string* error_message) override;
    void disconnect() override;

    bool test_connection(std::string* error_message) override;

    bool execute(const std::string& sql, std::string* error_message) override;

    bool query(
        const std::string&                    sql,
        std::vector<std::vector<std::string>>* rows,
        std::string*                          error_message
    ) override;

    bool get_table_metadata(
        const std::string& table_name,
        TableMetadata*     metadata,
        std::string*       error_message
    ) override;

    [[nodiscard]] bool supports_load_mode() const override;

private:
    bool run_query(
        const std::string&                    sql,
        std::vector<std::vector<std::string>>* rows,
        std::string*                          error_message
    ) const;

    bool load_table_metadata(
        const std::string& table_name,
        TableMetadata*     metadata,
        std::string*       error_message
    ) const;

    sqlite3*    db_ = nullptr;
    bool        connected_ = false;
    std::string db_path_;
};

}  // namespace data_generator::database

#endif
