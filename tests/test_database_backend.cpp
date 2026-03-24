/// @file test_database_backend.cpp

#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <vector>

#include <sqlite3.h>

#include "config/configuration.h"
#include "engine/executor.h"
#include "output/database_backend.h"
#include "output/database/db_url_parser.h"
#include "output/database/drivers/idatabase_driver.h"

using data_generator::config::GenerationConfig;
using data_generator::config::ParseMode;
using data_generator::config::ValidationIssue;

namespace {

GenerationConfig parse_or_fail(const nlohmann::json& root) {
    GenerationConfig cfg;
    std::vector<ValidationIssue> issues;
    const bool ok = data_generator::config::parse_generation_config(
        root,
        ParseMode::RequireOutputSettings,
        &cfg,
        &issues
    );
    EXPECT_TRUE(ok);
    EXPECT_TRUE(issues.empty());
    return cfg;
}

bool sqlite_exec(const std::filesystem::path& db_path, const std::string& sql, std::string* error) {
    sqlite3* db = nullptr;
    const int rc = sqlite3_open_v2(db_path.string().c_str(), &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);
    if (rc != SQLITE_OK) {
        if (error) { *error = db ? sqlite3_errmsg(db) : "sqlite open failed"; }
        if (db) { sqlite3_close(db); }
        return false;
    }

    char* error_text = nullptr;
    const int exec_rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &error_text);
    if (exec_rc != SQLITE_OK) {
        if (error) { *error = error_text ? error_text : sqlite3_errmsg(db); }
        if (error_text) { sqlite3_free(error_text); }
        sqlite3_close(db);
        return false;
    }

    sqlite3_close(db);
    return true;
}

int sqlite_count(const std::filesystem::path& db_path, std::string* error) {
    sqlite3* db = nullptr;
    const int rc = sqlite3_open_v2(db_path.string().c_str(), &db, SQLITE_OPEN_READONLY, nullptr);
    if (rc != SQLITE_OK) {
        if (error) { *error = db ? sqlite3_errmsg(db) : "sqlite open failed"; }
        if (db) { sqlite3_close(db); }
        return -1;
    }

    sqlite3_stmt* stmt = nullptr;
    const int prepare_rc = sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM t_data", -1, &stmt, nullptr);
    if (prepare_rc != SQLITE_OK) {
        if (error) { *error = sqlite3_errmsg(db); }
        sqlite3_close(db);
        return -1;
    }

    int count = -1;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0);
    } else if (error) {
        *error = sqlite3_errmsg(db);
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return count;
}

void ensure_tmp_dir(const std::filesystem::path& base) {
    std::error_code ec;
    std::filesystem::create_directories(base / "tmp", ec);
}

}  // namespace

TEST(DatabaseBackendTest, InsertAndBulkModesWithSqlite) {
    const auto db_path = std::filesystem::temp_directory_path() / "dg_backend.sqlite";
    std::error_code ec;
    std::filesystem::remove(db_path, ec);

    std::string error;
    ASSERT_TRUE(sqlite_exec(db_path,
                            "CREATE TABLE IF NOT EXISTS t_data (id INTEGER, name TEXT);",
                            &error)) << error;

    nlohmann::json base = {
        {"rows", 2},
        {"output", {
            {"type", "database"},
            {"database", {
                {"connection", std::string("sqlite://") + db_path.string()},
                {"table", "t_data"},
                {"insert_mode", "auto"},
                {"batch_size", 1},
                {"queue_size", 2},
                {"threads", 1},
                {"transaction_mode", "none"},
                {"error_policy", "stop"},
                {"rate_limit_rows_per_sec", 1000}
            }}
        }},
        {"fields", nlohmann::json::array({
            {{"name", "id"}, {"generator", "integer"}, {"config", {{"start", 1}, {"end", 9}}}},
            {{"name", "name"}, {"generator", "uuid"}, {"config", {{"include_hyphens", false}}}}
        })}
    };

    auto insert_cfg = parse_or_fail(base);
    data_generator::output::DatabaseBackend backend;
    data_generator::engine::ExecutionOptions options;

    ASSERT_TRUE(sqlite_exec(db_path, "DELETE FROM t_data;", &error)) << error;
    auto insert_stats = backend.generate(insert_cfg, options);
    EXPECT_EQ(insert_stats.rows_written, 2u);
    EXPECT_EQ(sqlite_count(db_path, &error), 2) << error;

    base["output"]["database"]["batch_size"] = 2;
    auto bulk_cfg = parse_or_fail(base);
    ASSERT_TRUE(sqlite_exec(db_path, "DELETE FROM t_data;", &error)) << error;
    auto bulk_stats = backend.generate(bulk_cfg, options);
    EXPECT_EQ(bulk_stats.rows_written, 2u);
    EXPECT_EQ(sqlite_count(db_path, &error), 2) << error;
}

TEST(DatabaseBackendTest, LoadModeUnsupportedRespectsContinuePolicy) {
    const auto db_path = std::filesystem::temp_directory_path() / "dg_backend_load.sqlite";
    std::error_code ec;
    std::filesystem::remove(db_path, ec);

    std::string error;
    ASSERT_TRUE(sqlite_exec(db_path,
                            "CREATE TABLE IF NOT EXISTS t_data (id INTEGER, name TEXT);",
                            &error)) << error;

    nlohmann::json root = {
        {"rows", 2},
        {"output", {
            {"type", "database"},
            {"database", {
                {"connection", std::string("sqlite://") + db_path.string()},
                {"table", "t_data"},
                {"insert_mode", "load"},
                {"batch_size", 2},
                {"queue_size", 2},
                {"threads", 1},
                {"transaction_mode", "per-batch"},
                {"error_policy", "continue"},
                {"rate_limit_rows_per_sec", 1}
            }}
        }},
        {"fields", nlohmann::json::array({
            {{"name", "id"}, {"generator", "integer"}, {"config", {{"start", 1}, {"end", 3}}}},
            {{"name", "name"}, {"generator", "uuid"}, {"config", {{"include_hyphens", false}}}}
        })}
    };

    auto cfg = parse_or_fail(root);
    cfg.workspace = (std::filesystem::temp_directory_path() / "dg_backend_workspace").string();
    ensure_tmp_dir(cfg.workspace);

    data_generator::output::DatabaseBackend backend;
    data_generator::engine::ExecutionOptions options;

    ASSERT_TRUE(sqlite_exec(db_path, "DELETE FROM t_data;", &error)) << error;
    auto stats = backend.generate(cfg, options);
    EXPECT_EQ(stats.rows_generated, 2u);
    EXPECT_EQ(stats.rows_written, 0u);
}

TEST(DatabaseBackendTest, ErrorPoliciesHandleInvalidOverrides) {
    const auto db_path = std::filesystem::temp_directory_path() / "dg_backend_error.sqlite";
    std::error_code ec;
    std::filesystem::remove(db_path, ec);

    std::string error;
    ASSERT_TRUE(sqlite_exec(db_path,
                            "CREATE TABLE IF NOT EXISTS t_data (id INTEGER, name TEXT);",
                            &error)) << error;

    nlohmann::json base = {
        {"rows", 2},
        {"output", {
            {"type", "database"},
            {"database", {
                {"connection", std::string("sqlite://") + db_path.string()},
                {"table", "t_data"},
                {"insert_mode", "insert"},
                {"batch_size", 2},
                {"queue_size", 2},
                {"threads", 2},
                {"transaction_mode", "per-run"},
                {"error_policy", "rollback-all"},
                {"rate_limit_rows_per_sec", 1}
            }}
        }},
        {"fields", nlohmann::json::array({
            {{"name", "id"}, {"generator", "integer"},
             {"config", {{"start", 1}, {"end", 3}}},
             {"default_value", {{"enabled", true}, {"percent", 100}, {"value", "bad"}}}},
            {{"name", "name"}, {"generator", "uuid"}, {"config", {{"include_hyphens", false}}}}
        })}
    };

    auto rollback_cfg = parse_or_fail(base);
    rollback_cfg.workspace = (std::filesystem::temp_directory_path() / "dg_backend_workspace2").string();
    ensure_tmp_dir(rollback_cfg.workspace);

    data_generator::output::DatabaseBackend backend;
    data_generator::engine::ExecutionOptions options;

    ASSERT_TRUE(sqlite_exec(db_path, "DELETE FROM t_data;", &error)) << error;
    auto rollback_stats = backend.generate(rollback_cfg, options);
    EXPECT_EQ(rollback_stats.rows_written, 0u);

    base["output"]["database"]["error_policy"] = "continue";
    base["output"]["database"]["transaction_mode"] = "per-batch";
    auto continue_cfg = parse_or_fail(base);
    continue_cfg.workspace = (std::filesystem::temp_directory_path() / "dg_backend_workspace3").string();
    ensure_tmp_dir(continue_cfg.workspace);

    ASSERT_TRUE(sqlite_exec(db_path, "DELETE FROM t_data;", &error)) << error;
    auto continue_stats = backend.generate(continue_cfg, options);
    EXPECT_EQ(continue_stats.rows_generated, 2u);
    EXPECT_EQ(continue_stats.rows_written, 0u);
}

TEST(DatabaseBackendTest, InvalidUrlAndMissingTableThrow) {
    nlohmann::json root = {
        {"rows", 1},
        {"output", {
            {"type", "database"},
            {"database", {
                {"connection", "not-a-valid-connection"},
                {"table", "t_data"},
                {"insert_mode", "insert"},
                {"batch_size", 1},
                {"queue_size", 1},
                {"threads", 1},
                {"transaction_mode", "none"},
                {"error_policy", "stop"},
                {"rate_limit_rows_per_sec", 1}
            }}
        }},
        {"fields", nlohmann::json::array({
            {{"name", "id"}, {"generator", "integer"}, {"config", {{"start", 1}, {"end", 2}}}}
        })}
    };

    auto cfg = parse_or_fail(root);
    data_generator::output::DatabaseBackend backend;
    data_generator::engine::ExecutionOptions options;

    EXPECT_THROW(backend.generate(cfg, options), std::runtime_error);

    cfg.output.database.connection = std::string("sqlite://") + std::filesystem::temp_directory_path().string() + "/tmp.db";
    cfg.output.database.table.clear();
    EXPECT_THROW(backend.generate(cfg, options), std::runtime_error);
}

TEST(DatabaseBackendTest, SchemaValidationFailureThrows) {
    const auto db_path = std::filesystem::temp_directory_path() / "dg_backend_schema.sqlite";
    std::error_code ec;
    std::filesystem::remove(db_path, ec);

    std::string error;
    ASSERT_TRUE(sqlite_exec(db_path,
                            "CREATE TABLE IF NOT EXISTS t_data (id INTEGER);",
                            &error)) << error;

    nlohmann::json root = {
        {"rows", 1},
        {"output", {
            {"type", "database"},
            {"database", {
                {"connection", std::string("sqlite://") + db_path.string()},
                {"table", "t_data"},
                {"insert_mode", "insert"},
                {"batch_size", 1},
                {"queue_size", 1},
                {"threads", 1},
                {"transaction_mode", "none"},
                {"error_policy", "stop"},
                {"rate_limit_rows_per_sec", 1}
            }}
        }},
        {"fields", nlohmann::json::array({
            {{"name", "id"},
             {"generator", "date"},
             {"config", {{"start_date", "2020-01-01"}, {"end_date", "2020-12-31"}}}}
        })}
    };

    auto cfg = parse_or_fail(root);
    data_generator::output::DatabaseBackend backend;
    data_generator::engine::ExecutionOptions options;

    EXPECT_THROW(backend.generate(cfg, options), std::runtime_error);
}

TEST(DatabaseBackendTest, StopPolicyThrowsOnConversionError) {
    const auto db_path = std::filesystem::temp_directory_path() / "dg_backend_stop.sqlite";
    std::error_code ec;
    std::filesystem::remove(db_path, ec);

    std::string error;
    ASSERT_TRUE(sqlite_exec(db_path,
                            "CREATE TABLE IF NOT EXISTS t_data (id INTEGER);",
                            &error)) << error;

    nlohmann::json root = {
        {"rows", 1},
        {"output", {
            {"type", "database"},
            {"database", {
                {"connection", std::string("sqlite://") + db_path.string()},
                {"table", "t_data"},
                {"insert_mode", "insert"},
                {"batch_size", 1},
                {"queue_size", 1},
                {"threads", 1},
                {"transaction_mode", "none"},
                {"error_policy", "stop"},
                {"rate_limit_rows_per_sec", 1}
            }}
        }},
        {"fields", nlohmann::json::array({
            {{"name", "id"},
             {"generator", "integer"},
             {"config", {{"start", 1}, {"end", 9}}},
             {"default_value", {{"enabled", true}, {"percent", 100}, {"value", "bad"}}}}
        })}
    };

    auto cfg = parse_or_fail(root);
    data_generator::output::DatabaseBackend backend;
    data_generator::engine::ExecutionOptions options;

    EXPECT_THROW(backend.generate(cfg, options), std::runtime_error);
}

TEST(DatabaseBackendTest, RollbackBatchPolicyContinuesOnConversionError) {
    const auto db_path = std::filesystem::temp_directory_path() / "dg_backend_rollback_batch.sqlite";
    std::error_code ec;
    std::filesystem::remove(db_path, ec);

    std::string error;
    ASSERT_TRUE(sqlite_exec(db_path,
                            "CREATE TABLE IF NOT EXISTS t_data (id INTEGER);",
                            &error)) << error;

    nlohmann::json root = {
        {"rows", 2},
        {"output", {
            {"type", "database"},
            {"database", {
                {"connection", std::string("sqlite://") + db_path.string()},
                {"table", "t_data"},
                {"insert_mode", "insert"},
                {"batch_size", 2},
                {"queue_size", 1},
                {"threads", 1},
                {"transaction_mode", "per-batch"},
                {"error_policy", "rollback-batch"},
                {"rate_limit_rows_per_sec", 1}
            }}
        }},
        {"fields", nlohmann::json::array({
            {{"name", "id"},
             {"generator", "integer"},
             {"config", {{"start", 1}, {"end", 9}}},
             {"default_value", {{"enabled", true}, {"percent", 100}, {"value", "bad"}}}}
        })}
    };

    auto cfg = parse_or_fail(root);
    data_generator::output::DatabaseBackend backend;
    data_generator::engine::ExecutionOptions options;

    const auto stats = backend.generate(cfg, options);
    EXPECT_EQ(stats.rows_generated, 2u);
}

TEST(DatabaseBackendTest, PostgresOdbcBulkInsertMode) {
    const char* pg_url = std::getenv("DATA_GENERATOR_TEST_PG_URL");
    if (!pg_url || std::string(pg_url).empty()) {
        GTEST_SKIP() << "DATA_GENERATOR_TEST_PG_URL not set; skipping ODBC Postgres coverage.";
    }

    data_generator::database::DbUrl parsed;
    std::string error;
    if (!data_generator::database::parse_db_connection(pg_url, &parsed, &error)) {
        GTEST_SKIP() << "DATA_GENERATOR_TEST_PG_URL is not in the new connection format: " << error;
    }

    auto driver = data_generator::database::make_database_driver(parsed.type);
    ASSERT_TRUE(driver);
    ASSERT_TRUE(driver->connect(parsed, &error)) << error;

    ASSERT_TRUE(driver->execute(
        "CREATE TABLE IF NOT EXISTS t_data (id INT, name TEXT);",
        &error
    )) << error;
    ASSERT_TRUE(driver->execute("TRUNCATE TABLE t_data;", &error)) << error;

    driver->disconnect();

    nlohmann::json root = {
        {"rows", 3},
        {"output", {
            {"type", "database"},
            {"database", {
                {"connection", pg_url},
                {"table", "t_data"},
                {"insert_mode", "bulk"},
                {"batch_size", 2},
                {"queue_size", 2},
                {"threads", 1},
                {"transaction_mode", "per-batch"},
                {"error_policy", "stop"},
                {"rate_limit_rows_per_sec", 1}
            }}
        }},
        {"fields", nlohmann::json::array({
            {{"name", "id"}, {"generator", "integer"}, {"config", {{"start", 1}, {"end", 9}}}},
            {{"name", "name"}, {"generator", "uuid"}, {"config", {{"include_hyphens", false}}}}
        })}
    };

    auto cfg = parse_or_fail(root);
    cfg.workspace = (std::filesystem::temp_directory_path() / "dg_backend_pg_workspace").string();
    ensure_tmp_dir(cfg.workspace);

    data_generator::output::DatabaseBackend backend;
    data_generator::engine::ExecutionOptions options;

    auto stats = backend.generate(cfg, options);
    EXPECT_EQ(stats.rows_written, 3u);

    ASSERT_TRUE(driver->connect(parsed, &error)) << error;
    std::vector<std::vector<std::string>> rows;
    ASSERT_TRUE(driver->query("SELECT COUNT(*) FROM t_data;", &rows, &error)) << error;
    ASSERT_FALSE(rows.empty());
    ASSERT_FALSE(rows.front().empty());
    EXPECT_EQ(std::stoi(rows.front().front()), 3);
    driver->disconnect();
}
