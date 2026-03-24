/// @file test_run.cpp

#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include <sqlite3.h>

#include "app/run.h"
#include "cli/exit_codes.h"

using namespace data_generator;

namespace {

int invoke_cli(const std::vector<std::string>& args) {
    std::vector<std::string> storage;
    storage.reserve(args.size() + 1);
    storage.emplace_back("data-generator");
    for (const auto& arg : args) {
        storage.push_back(arg);
    }

    std::vector<char*> argv;
    argv.reserve(storage.size());
    for (auto& s : storage) {
        argv.push_back(s.data());
    }

    return run(static_cast<int>(argv.size()), argv.data());
}

std::filesystem::path test_input(const std::string& filename) {
    return std::filesystem::path(DATA_GENERATOR_TEST_INPUT_DIR) / filename;
}

bool create_sqlite_table(const std::filesystem::path& db_path, std::string* error_message) {
    sqlite3* db = nullptr;
    const int rc = sqlite3_open_v2(db_path.string().c_str(), &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);
    if (rc != SQLITE_OK) {
        if (error_message) { *error_message = db ? sqlite3_errmsg(db) : "sqlite open failed"; }
        if (db) { sqlite3_close(db); }
        return false;
    }

    const char* sql = "CREATE TABLE IF NOT EXISTS t_data (id INTEGER, uid TEXT);";
    char* error_text = nullptr;
    const int exec_rc = sqlite3_exec(db, sql, nullptr, nullptr, &error_text);
    if (exec_rc != SQLITE_OK) {
        if (error_message) {
            *error_message = error_text ? error_text : sqlite3_errmsg(db);
        }
        if (error_text) { sqlite3_free(error_text); }
        sqlite3_close(db);
        return false;
    }

    sqlite3_close(db);
    return true;
}

int sqlite_count_rows(const std::filesystem::path& db_path, std::string* error_message) {
    sqlite3* db = nullptr;
    const int rc = sqlite3_open_v2(db_path.string().c_str(), &db, SQLITE_OPEN_READONLY, nullptr);
    if (rc != SQLITE_OK) {
        if (error_message) { *error_message = db ? sqlite3_errmsg(db) : "sqlite open failed"; }
        if (db) { sqlite3_close(db); }
        return -1;
    }

    sqlite3_stmt* stmt = nullptr;
    const int prepare_rc = sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM t_data", -1, &stmt, nullptr);
    if (prepare_rc != SQLITE_OK) {
        if (error_message) { *error_message = sqlite3_errmsg(db); }
        sqlite3_close(db);
        return -1;
    }

    int count = -1;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0);
    } else if (error_message) {
        *error_message = sqlite3_errmsg(db);
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return count;
}

TEST(RunTest, HelpCommandReturnsOk) {
    EXPECT_EQ(invoke_cli({"help"}), cli::exit_codes::kOk);
}

TEST(RunTest, RunFromInputFileReturnsOkAndWritesOutput) {
    const auto input  = test_input("run_valid.json");
    const auto workspace = std::filesystem::temp_directory_path() / "data-generator-test-workspace";
    const auto output = workspace / "data-generator-test-output.csv";

    std::error_code ec;
    std::filesystem::remove(output, ec);
    std::filesystem::create_directories(workspace, ec);

    const int rc = invoke_cli({
        "run",
        input.string(),
        "--output",
        output.string(),
        "--rows",
        "16",
    });

    EXPECT_EQ(rc, cli::exit_codes::kOk);
    EXPECT_TRUE(std::filesystem::exists(output));

    std::ifstream generated(output);
    std::string data;
    std::getline(generated, data);
    EXPECT_FALSE(data.empty());

    std::filesystem::remove(output, ec);
}

TEST(RunTest, CheckMalformedJsonReturnsRuntimeFailure) {
    const auto input = test_input("malformed.json");
    EXPECT_EQ(
        invoke_cli({"check", input.string()}),
        cli::exit_codes::kRuntimeFailure
    );
}

TEST(RunTest, CheckMissingFieldsReturnsRuntimeFailure) {
    const auto input = test_input("missing_fields.json");
    EXPECT_EQ(
        invoke_cli({"check", input.string()}),
        cli::exit_codes::kRuntimeFailure
    );
}

TEST(RunTest, RunSQLiteDatabaseOutput) {
    const auto db_path = std::filesystem::temp_directory_path() / "dg_sqlite_test.db";
    std::error_code ec;
    std::filesystem::remove(db_path, ec);

    std::string error;
    ASSERT_TRUE(create_sqlite_table(db_path, &error)) << error;

    nlohmann::json root;
    root["rows"] = 3;
    root["output"] = {
        {"type", "database"},
        {"database", {
            {"connection", std::string("sqlite://") + db_path.string()},
            {"table", "t_data"},
            {"insert_mode", "insert"},
            {"batch_size", 2},
            {"queue_size", 2},
            {"threads", 1},
            {"transaction_mode", "per-batch"},
            {"error_policy", "stop"},
            {"rate_limit_rows_per_sec", 1000}
        }}
    };
    root["fields"] = nlohmann::json::array({
        {{"name", "id"}, {"generator", "integer"}, {"config", {{"start", 1}, {"end", 10}}}},
        {{"name", "uid"}, {"generator", "uuid"}, {"config", {{"include_hyphens", true}}}}
    });

    const auto config_path = std::filesystem::temp_directory_path() / "dg_cli_sqlite_config.json";
    {
        std::ofstream out(config_path, std::ios::trunc);
        out << root.dump(2);
    }

    EXPECT_EQ(invoke_cli({"run", config_path.string()}), cli::exit_codes::kOk);
    const int count = sqlite_count_rows(db_path, &error);
    EXPECT_EQ(count, 3) << error;
}

}  // namespace
