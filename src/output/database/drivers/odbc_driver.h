// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file odbc_driver.h

#ifndef DATAGEN_ODBC_DRIVER_H
#define DATAGEN_ODBC_DRIVER_H

#include <string>
#include <vector>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#include <sql.h>
#include <sqlext.h>

#include "output/database/drivers/idatabase_driver.h"

namespace datagen::database {

class OdbcDriver final : public IDatabaseDriver {
public:
    explicit OdbcDriver(DbType db_type = DbType::Odbc);
    ~OdbcDriver() override;

    [[nodiscard]] DbType      type() const override;
    [[nodiscard]] std::string dbms_name() const override;
    [[nodiscard]] std::string dbms_version() const override;

    bool connect(const DbUrl& url, std::string* error_message) override;
    void disconnect() override;

    bool test_connection(std::string* error_message) override;

    bool execute(const std::string& sql, std::string* error_message) override;

    bool
        query(const std::string& sql, std::vector<std::vector<std::string>>* rows, std::string* error_message) override;

    bool
        get_table_metadata(const std::string& table_name, TableMetadata* metadata, std::string* error_message) override;

    [[nodiscard]] bool supports_load_mode() const override;

private:
    static bool execute_statement(SQLHSTMT stmt, const std::string& sql, std::string* error_message);
    bool        run_query(
               const std::string&                     sql,
               std::vector<std::vector<std::string>>* rows,
               std::string*                           error_message
           ) const;

    bool load_mysql_metadata(const std::string& table_name, TableMetadata* metadata, std::string* error_message) const;
    bool load_postgresql_metadata(
        const std::string& table_name,
        TableMetadata*     metadata,
        std::string*       error_message
    ) const;
    bool load_oracle_metadata(const std::string& table_name, TableMetadata* metadata, std::string* error_message) const;

    DbType      db_type_ = DbType::Unknown;
    DbUrl       connection_;
    std::string dbms_name_;
    std::string dbms_version_;
    bool        connected_ = false;
    SQLHENV     env_       = SQL_NULL_HENV;
    SQLHDBC     dbc_       = SQL_NULL_HDBC;
};

}  // namespace datagen::database

#endif
