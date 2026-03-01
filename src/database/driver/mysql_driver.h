// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

#ifndef DATA_GENERATOR_DATABASE_DRIVER_MYSQL_DRIVER_H
#define DATA_GENERATOR_DATABASE_DRIVER_MYSQL_DRIVER_H

#include <string>
#include <vector>

#include "database/driver/idatabase_driver.h"

namespace data_generator::database {

class MysqlDriver final : public IDatabaseDriver {
public:
    DbType type() const override;

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

    bool supports_load_mode() const override;

private:
    bool run_mysql_command(
        const std::string& sql,
        bool               expect_rows,
        std::string*       stdout_text,
        std::string*       error_message
    ) const;

    static std::vector<std::string> split_tab_line(const std::string& line);

    DbUrl connection_;
    bool  connected_ = false;
};

}  // namespace data_generator::database

#endif
