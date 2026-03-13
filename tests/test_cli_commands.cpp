/// @file test_cli_commands.cpp

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

std::filesystem::path write_json_file(const std::string& name, const std::string& payload) {
    const auto path = std::filesystem::temp_directory_path() / name;
    std::ofstream out(path, std::ios::trunc);
    out << payload;
    out.close();
    return path;
}

nlohmann::json read_json_file(const std::filesystem::path& path) {
    std::ifstream input(path);
    nlohmann::json value;
    input >> value;
    return value;
}

bool create_sqlite_table(const std::filesystem::path& db_path, std::string* error_message) {
    sqlite3* db = nullptr;
    const int rc = sqlite3_open_v2(db_path.string().c_str(), &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);
    if (rc != SQLITE_OK) {
        if (error_message) { *error_message = db ? sqlite3_errmsg(db) : "sqlite open failed"; }
        if (db) { sqlite3_close(db); }
        return false;
    }

    const char* sql = "CREATE TABLE IF NOT EXISTS t_data (id INTEGER, name TEXT);";
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

const char* kValidConfigJson = R"json({
  "rows": 2,
  "output": {
    "type": "file",
    "file": {
      "format": "csv",
      "options": {
        "header": true,
        "line_ending": "LF"
      }
    }
  },
  "fields": [
    {
      "name": "id",
      "generator": "integer",
      "config": {
        "start": 1,
        "end": 9
      }
    }
  ]
})json";

const char* kInvalidConfigJson = R"json({
  "rows": 2,
  "output": {
    "type": "file",
    "file": {
      "format": "csv"
    }
  },
  "fields": []
})json";

TEST(CliCommandsTest, InfoDriversAndVersion) {
    EXPECT_EQ(invoke_cli({"info"}), cli::exit_codes::kOk);
    EXPECT_EQ(invoke_cli({"info", "--json"}), cli::exit_codes::kOk);
    EXPECT_EQ(invoke_cli({"info", "integer"}), cli::exit_codes::kOk);
    EXPECT_EQ(invoke_cli({"info", "integer", "--json"}), cli::exit_codes::kOk);
    EXPECT_EQ(invoke_cli({"info", "not_exist"}), cli::exit_codes::kUsage);

    EXPECT_EQ(invoke_cli({"drivers"}), cli::exit_codes::kOk);
    EXPECT_EQ(invoke_cli({"drivers", "--json"}), cli::exit_codes::kOk);

    EXPECT_EQ(invoke_cli({"-v"}), cli::exit_codes::kOk);
    EXPECT_EQ(invoke_cli({"--version"}), cli::exit_codes::kOk);
    EXPECT_EQ(invoke_cli({"version"}), cli::exit_codes::kOk);
}

TEST(CliCommandsTest, InitPreviewCheckAndSchema) {
    const auto init_out = std::filesystem::temp_directory_path() / "dg_cli_init_template.json";
    EXPECT_EQ(
        invoke_cli({"init", init_out.string(), "--template", "file", "--format", "csv"}),
        cli::exit_codes::kOk
    );
    EXPECT_TRUE(std::filesystem::exists(init_out));

    EXPECT_EQ(invoke_cli({"preview", init_out.string()}), cli::exit_codes::kOk);
    EXPECT_EQ(invoke_cli({"preview", init_out.string(), "--field", "id"}), cli::exit_codes::kOk);
    EXPECT_EQ(invoke_cli({"preview", init_out.string(), "--field", "missing"}), cli::exit_codes::kRuntimeFailure);

    EXPECT_EQ(invoke_cli({"check", init_out.string()}), cli::exit_codes::kOk);

    const auto invalid_input = write_json_file("dg_cli_invalid.json", "{");
    EXPECT_EQ(invoke_cli({"check", invalid_input.string()}), cli::exit_codes::kRuntimeFailure);

    const auto invalid_schema = write_json_file("dg_cli_invalid_schema.json", kInvalidConfigJson);
    EXPECT_EQ(invoke_cli({"check", invalid_schema.string()}), cli::exit_codes::kRuntimeFailure);

    const auto schema_out = std::filesystem::temp_directory_path() / "dg_cli_schema.json";
    EXPECT_EQ(invoke_cli({"schema", schema_out.string()}), cli::exit_codes::kOk);
    EXPECT_TRUE(std::filesystem::exists(schema_out));
}

TEST(CliCommandsTest, InitFormatsWarningsAndInference) {
    const auto temp_dir = std::filesystem::temp_directory_path();

    EXPECT_EQ(
        invoke_cli({"init", (temp_dir / "dg_init_bad_template.json").string(), "--template", "bad"}),
        cli::exit_codes::kUsage
    );

    EXPECT_EQ(
        invoke_cli({"init", (temp_dir / "dg_init_bad_format.json").string(), "--format", "bad"}),
        cli::exit_codes::kUsage
    );

    const auto csv_out = temp_dir / "dg_init_csv.json";
    EXPECT_EQ(
        invoke_cli({"init", csv_out.string(), "--template", "file", "--format", "csv"}),
        cli::exit_codes::kOk
    );
    auto csv_json = read_json_file(csv_out);
    EXPECT_EQ(csv_json["output"]["type"], "file");
    EXPECT_EQ(csv_json["output"]["file"]["format"], "csv");
    EXPECT_TRUE(csv_json["output"]["file"]["options"].contains("header"));

    const auto sql_out = temp_dir / "dg_init_sql.json";
    EXPECT_EQ(
        invoke_cli({"init", sql_out.string(), "--template", "file", "--format", "sql", "--table", "t_data"}),
        cli::exit_codes::kOk
    );
    auto sql_json = read_json_file(sql_out);
    EXPECT_EQ(sql_json["output"]["file"]["format"], "sql");
    EXPECT_EQ(sql_json["output"]["file"]["options"]["table"], "t_data");

    const auto custom_out = temp_dir / "dg_init_custom.json";
    EXPECT_EQ(
        invoke_cli({"init", custom_out.string(), "--template", "file", "--format", "Custom", "--table", "ignored"}),
        cli::exit_codes::kOk
    );
    auto custom_json = read_json_file(custom_out);
    EXPECT_EQ(custom_json["output"]["file"]["format"], "Custom");
    EXPECT_FALSE(custom_json["output"]["file"]["options"].contains("table"));

    const auto db_out = temp_dir / "dg_init_db.json";
    EXPECT_EQ(
        invoke_cli({"init", db_out.string(), "--template", "database", "--format", "csv"}),
        cli::exit_codes::kOk
    );

    const auto db_warn_out = temp_dir / "dg_init_db_warn.json";
    EXPECT_EQ(
        invoke_cli({"init", db_warn_out.string(), "--template", "database", "--from-database", "odbc:postgresql:DRIVER={PostgreSQL 17 ODBC driver};"}),
        cli::exit_codes::kOk
    );

    const auto db_bad_out = temp_dir / "dg_init_db_bad.json";
    EXPECT_EQ(
        invoke_cli({"init", db_bad_out.string(), "--template", "database", "--from-database", "bad", "--table", "t_data"}),
        cli::exit_codes::kUsage
    );

    const auto sqlite_path = temp_dir / "dg_init_infer.sqlite";
    std::error_code ec;
    std::filesystem::remove(sqlite_path, ec);
    std::string error;
    ASSERT_TRUE(create_sqlite_table(sqlite_path, &error)) << error;

    const auto infer_out = temp_dir / "dg_init_infer.json";
    EXPECT_EQ(
        invoke_cli({
            "init",
            infer_out.string(),
            "--template",
            "database",
            "--from-database",
            std::string("sqlite:") + sqlite_path.string(),
            "--table",
            "t_data"
        }),
        cli::exit_codes::kOk
    );

    auto infer_json = read_json_file(infer_out);
    EXPECT_TRUE(infer_json.contains("fields"));
    EXPECT_FALSE(infer_json["fields"].empty());
}

TEST(CliCommandsTest, RunErrorAndHelpBranches) {
    const auto valid_input = write_json_file("dg_cli_valid_run.json", kValidConfigJson);
    const auto output_path = std::filesystem::temp_directory_path() / "dg_cli_out.csv";

    EXPECT_EQ(invoke_cli({"run"}), cli::exit_codes::kUsage);
    EXPECT_EQ(invoke_cli({"run", valid_input.string(), "--rows", "0"}), cli::exit_codes::kUsage);

    EXPECT_EQ(
        invoke_cli({"run", valid_input.string(), "--output", output_path.string()}),
        cli::exit_codes::kOk
    );
    EXPECT_TRUE(std::filesystem::exists(output_path));

    EXPECT_EQ(invoke_cli({"run", "--help"}), cli::exit_codes::kOk);
    EXPECT_EQ(invoke_cli({"preview", "--help"}), cli::exit_codes::kOk);
    EXPECT_EQ(invoke_cli({"check", "--help"}), cli::exit_codes::kOk);
    EXPECT_EQ(invoke_cli({"info", "--help"}), cli::exit_codes::kOk);
    EXPECT_EQ(invoke_cli({"init", "--help"}), cli::exit_codes::kOk);
    EXPECT_EQ(invoke_cli({"drivers", "--help"}), cli::exit_codes::kOk);
    EXPECT_EQ(invoke_cli({"schema", "--help"}), cli::exit_codes::kOk);
}

TEST(CliCommandsTest, DispatchUnknownCommand) {
    EXPECT_EQ(invoke_cli({"unknown-command"}), cli::exit_codes::kOk);
}

}  // namespace
