// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file odbc_driver.h

#ifndef DATA_GENERATOR_ODBC_DRIVER_H
#define DATA_GENERATOR_ODBC_DRIVER_H

#include <string>
#include <vector>

#include <sql.h>
#include <sqlext.h>

#include "database/driver/idatabase_driver.h"

namespace data_generator::database {

class OdbcDriver final : public IDatabaseDriver {
public:
    explicit OdbcDriver(DbType db_type);
    ~OdbcDriver() override;

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
    bool execute_statement(SQLHSTMT stmt, const std::string& sql, std::string* error_message) const;
    bool run_query(
        const std::string&                    sql,
        std::vector<std::vector<std::string>>* rows,
        std::string*                          error_message
    ) const;

    bool load_mysql_metadata(const std::string& table_name, TableMetadata* metadata, std::string* error_message);
    bool load_postgresql_metadata(const std::string& table_name, TableMetadata* metadata, std::string* error_message);
    bool load_oracle_metadata(const std::string& table_name, TableMetadata* metadata, std::string* error_message);

    DbType       db_type_ = DbType::Unknown;
    DbUrl        connection_;
    bool         connected_ = false;
    SQLHENV      env_ = SQL_NULL_HENV;
    SQLHDBC      dbc_ = SQL_NULL_HDBC;
};

}  // namespace data_generator::database

#endif
