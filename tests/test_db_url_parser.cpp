/// @file test_db_url_parser.cpp

#include <gtest/gtest.h>

#include <string>

#include "output/database/db_url_parser.h"

using data_generator::database::DbType;
using data_generator::database::DbUrl;
using data_generator::database::parse_db_connection;

TEST(DbUrlParserTest, RejectsNullOutputPointer) {
    std::string error;
    EXPECT_FALSE(parse_db_connection("sqlite://:memory:", nullptr, &error));
    EXPECT_FALSE(error.empty());
}

TEST(DbUrlParserTest, ParsesSqliteUrls) {
    DbUrl parsed;
    std::string error;

    EXPECT_TRUE(parse_db_connection("sqlite://:memory:", &parsed, &error)) << error;
    EXPECT_EQ(parsed.type, DbType::Sqlite);
    EXPECT_EQ(parsed.database, ":memory:");

    EXPECT_TRUE(parse_db_connection("sqlite:///tmp/my.db", &parsed, &error)) << error;
    EXPECT_EQ(parsed.type, DbType::Sqlite);
    EXPECT_EQ(parsed.database, "/tmp/my.db");
}

TEST(DbUrlParserTest, ParsesOdbcPrefixedUrls) {
    DbUrl parsed;
    std::string error;

    const std::string odbc =
        "odbc://DRIVER={PostgreSQL 17 ODBC driver};SERVER=localhost;PORT=5432;DATABASE=test;UID=user;PWD=pass;";
    EXPECT_TRUE(parse_db_connection(odbc, &parsed, &error)) << error;
    EXPECT_EQ(parsed.type, DbType::Odbc);
    EXPECT_EQ(parsed.odbc_connection_string,
              "DRIVER={PostgreSQL 17 ODBC driver};SERVER=localhost;PORT=5432;DATABASE=test;UID=user;PWD=pass;");
}

TEST(DbUrlParserTest, RejectsInvalidSqliteAndUnknownStrings) {
    DbUrl parsed;
    std::string error;

    EXPECT_FALSE(parse_db_connection("sqlite://", &parsed, &error));
    EXPECT_FALSE(error.empty());

    EXPECT_FALSE(parse_db_connection("not-a-valid-connection", &parsed, &error));
    EXPECT_FALSE(error.empty());

    EXPECT_FALSE(parse_db_connection("sqlite://relative.db", &parsed, &error));
    EXPECT_NE(error.find("sqlite:///path/to/file.db"), std::string::npos);
}
