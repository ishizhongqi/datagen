/// @file test_odbc_driver.cpp

#include <gtest/gtest.h>

#include <cstdlib>
#include <string>
#include <vector>

#include "output/database/db_url_parser.h"
#include "output/database/drivers/odbc_driver.h"

using data_generator::database::DbUrl;
using data_generator::database::OdbcDriver;
using data_generator::database::parse_db_url;

TEST(OdbcDriverTest, ConnectQueryAndMetadataForPostgres) {
    const char* pg_url = std::getenv("DATA_GENERATOR_TEST_PG_URL");
    if (!pg_url || std::string(pg_url).empty()) {
        GTEST_SKIP() << "DATA_GENERATOR_TEST_PG_URL not set.";
    }

    DbUrl parsed;
    std::string error;
    ASSERT_TRUE(parse_db_url(pg_url, &parsed, &error)) << error;

    OdbcDriver driver(parsed.type);
    EXPECT_FALSE(driver.test_connection(&error));

    ASSERT_TRUE(driver.connect(parsed, &error)) << error;
    EXPECT_TRUE(driver.test_connection(&error));
    EXPECT_TRUE(driver.supports_load_mode());

    ASSERT_TRUE(driver.execute("CREATE TABLE IF NOT EXISTS odbc_test (id INT, name TEXT);", &error)) << error;
    ASSERT_TRUE(driver.execute("TRUNCATE TABLE odbc_test;", &error)) << error;
    ASSERT_TRUE(driver.execute("INSERT INTO odbc_test (id, name) VALUES (1, 'alpha');", &error)) << error;

    std::vector<std::vector<std::string>> rows;
    ASSERT_TRUE(driver.query("SELECT id, name FROM odbc_test ORDER BY id;", &rows, &error)) << error;
    ASSERT_FALSE(rows.empty());
    EXPECT_EQ(rows[0][0], "1");

    data_generator::database::TableMetadata metadata;
    ASSERT_TRUE(driver.get_table_metadata("odbc_test", &metadata, &error)) << error;
    EXPECT_EQ(metadata.table_name, "odbc_test");
    EXPECT_FALSE(metadata.columns.empty());

    EXPECT_FALSE(driver.query("SELECT 1", nullptr, &error));
    EXPECT_FALSE(error.empty());

    driver.disconnect();
}
