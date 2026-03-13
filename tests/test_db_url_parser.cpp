/// @file test_db_url_parser.cpp

#include <gtest/gtest.h>

#include <cstdlib>
#include <string>

#include "output/database/db_url_parser.h"

using data_generator::database::DbType;
using data_generator::database::DbUrl;
using data_generator::database::build_db_url;
using data_generator::database::parse_db_url;

namespace {

void set_env(const char* key, const char* value) {
    if (value) {
        ::setenv(key, value, 1);
    } else {
        ::unsetenv(key);
    }
}

}  // namespace

TEST(DbUrlParserTest, RejectsNullOutputPointer) {
    std::string error;
    EXPECT_FALSE(parse_db_url("sqlite::memory:", nullptr, &error));
    EXPECT_FALSE(error.empty());
}

TEST(DbUrlParserTest, ParsesSqliteUrls) {
    DbUrl parsed;
    std::string error;

    EXPECT_TRUE(parse_db_url("sqlite::memory:", &parsed, &error)) << error;
    EXPECT_EQ(parsed.type, DbType::Sqlite);
    EXPECT_EQ(parsed.database, ":memory:");

    EXPECT_TRUE(parse_db_url("sqlite:/tmp/my%20db.sqlite", &parsed, &error)) << error;
    EXPECT_EQ(parsed.type, DbType::Sqlite);
    EXPECT_EQ(parsed.database, "/tmp/my db.sqlite");
}

TEST(DbUrlParserTest, ParsesOdbcPrefixedUrls) {
    DbUrl parsed;
    std::string error;

    const std::string odbc =
        "odbc:postgresql:DRIVER={PostgreSQL 17 ODBC driver};SERVER=localhost;PORT=5432;DATABASE=test;UID=user;PWD=pass;";
    EXPECT_TRUE(parse_db_url(odbc, &parsed, &error)) << error;
    EXPECT_EQ(parsed.type, DbType::Postgresql);
    EXPECT_EQ(parsed.odbc_connection_string,
              "DRIVER={PostgreSQL 17 ODBC driver};SERVER=localhost;PORT=5432;DATABASE=test;UID=user;PWD=pass;");

    EXPECT_FALSE(parse_db_url("odbc:unknown:DRIVER={X};", &parsed, &error));
    EXPECT_FALSE(error.empty());
}

TEST(DbUrlParserTest, ParsesLegacyAndInferenceUrls) {
    DbUrl parsed;
    std::string error;

    EXPECT_TRUE(parse_db_url("postgresql://user:pass@localhost:5432/sample", &parsed, &error)) << error;
    EXPECT_EQ(parsed.type, DbType::Postgresql);
    EXPECT_EQ(parsed.username, "user");
    EXPECT_EQ(parsed.password, "pass");
    EXPECT_EQ(parsed.host, "localhost");
    EXPECT_EQ(parsed.port, 5432);
    EXPECT_EQ(parsed.database, "sample");
    EXPECT_FALSE(parsed.odbc_connection_string.empty());

    EXPECT_TRUE(parse_db_url("DRIVER={PostgreSQL 17 ODBC driver};DATABASE=demo;", &parsed, &error)) << error;
    EXPECT_EQ(parsed.type, DbType::Postgresql);
    EXPECT_EQ(parsed.odbc_connection_string, "DRIVER={PostgreSQL 17 ODBC driver};DATABASE=demo;");

    EXPECT_FALSE(parse_db_url("postgresql://user:pass@localhost:badport/db", &parsed, &error));
    EXPECT_FALSE(error.empty());
}

TEST(DbUrlParserTest, BuildsDbUrlForSqliteAndPostgres) {
    set_env("DATA_GENERATOR_ODBC_DRIVER_POSTGRESQL", "PostgreSQL 17 ODBC driver");

    const std::string sqlite_memory = build_db_url(DbType::Sqlite, "", "", "", std::nullopt, ":memory:");
    EXPECT_EQ(sqlite_memory, "sqlite::memory:");

    const std::string sqlite_path = build_db_url(DbType::Sqlite, "", "", "", std::nullopt, "/tmp/a b.db");
    EXPECT_EQ(sqlite_path, "sqlite:%2Ftmp%2Fa%20b.db");

    const std::string pg_url = build_db_url(DbType::Postgresql, "user", "pass", "localhost", 5433, "demo");
    EXPECT_FALSE(pg_url.empty());
    EXPECT_TRUE(pg_url.rfind("odbc:postgresql:", 0) == 0);
}
