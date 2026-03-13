/// @file test_sqlite_driver.cpp

#include <gtest/gtest.h>

#include <filesystem>
#include <string>
#include <vector>

#include "output/database/drivers/sqlite_driver.h"

using data_generator::database::DbType;
using data_generator::database::DbUrl;
using data_generator::database::SqliteDriver;
using data_generator::database::TableMetadata;

namespace {

DbUrl make_sqlite_url(const std::filesystem::path& path) {
    DbUrl url;
    url.type = DbType::Sqlite;
    url.raw = std::string("sqlite:") + path.string();
    url.database = path.string();
    return url;
}

}  // namespace

TEST(SqliteDriverTest, ConnectionAndMetadataCoverage) {
    const auto db_path = std::filesystem::temp_directory_path() / "dg_sqlite_driver_test.db";
    std::error_code ec;
    std::filesystem::remove(db_path, ec);

    SqliteDriver driver;
    std::string error;

    DbUrl bad_type;
    bad_type.type = DbType::Postgresql;
    EXPECT_FALSE(driver.connect(bad_type, &error));
    EXPECT_FALSE(error.empty());

    DbUrl bad_path;
    bad_path.type = DbType::Sqlite;
    bad_path.database.clear();
    EXPECT_FALSE(driver.connect(bad_path, &error));

    const DbUrl url = make_sqlite_url(db_path);
    ASSERT_TRUE(driver.connect(url, &error)) << error;
    EXPECT_TRUE(driver.test_connection(&error)) << error;

    EXPECT_FALSE(driver.query("SELECT 1", nullptr, &error));

    ASSERT_TRUE(driver.execute("PRAGMA foreign_keys=ON;", &error)) << error;
    ASSERT_TRUE(driver.execute("CREATE TABLE parent (id INTEGER PRIMARY KEY AUTOINCREMENT);", &error)) << error;
    ASSERT_TRUE(driver.execute(
        "CREATE TABLE child (id INTEGER PRIMARY KEY AUTOINCREMENT, parent_id INTEGER, name VARCHAR(5), "
        "created_at DATETIME DEFAULT CURRENT_TIMESTAMP, "
        "FOREIGN KEY(parent_id) REFERENCES parent(id));",
        &error
    )) << error;
    ASSERT_TRUE(driver.execute("CREATE INDEX idx_child_name ON child(name);", &error)) << error;
    ASSERT_TRUE(driver.execute(
        "CREATE TRIGGER trg_child AFTER INSERT ON child BEGIN "
        "UPDATE child SET name=name WHERE id=NEW.id; END;",
        &error
    )) << error;

    TableMetadata meta;
    ASSERT_TRUE(driver.get_table_metadata("child", &meta, &error)) << error;
    EXPECT_EQ(meta.table_name, "child");
    EXPECT_FALSE(meta.columns.empty());
    EXPECT_FALSE(meta.foreign_keys.empty());
    EXPECT_FALSE(meta.indexes.empty());
    EXPECT_FALSE(meta.triggers.empty());

    TableMetadata missing;
    EXPECT_FALSE(driver.get_table_metadata("missing_table", &missing, &error));

    driver.disconnect();

    std::vector<std::vector<std::string>> rows;
    EXPECT_FALSE(driver.query("SELECT 1", &rows, &error));
}
