/// @file test_cli_commands.cpp

#include <algorithm>

#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include <cstdlib>
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

const nlohmann::json* find_field_by_name(const nlohmann::json& fields, const std::string& name) {
    const auto it = std::find_if(fields.begin(), fields.end(), [&](const nlohmann::json& field) {
        return field.value("name", "") == name;
    });
    if (it == fields.end()) { return nullptr; }
    return &(*it);
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

bool create_empty_sqlite_db(const std::filesystem::path& db_path, std::string* error_message) {
    sqlite3* db = nullptr;
    const int rc = sqlite3_open_v2(db_path.string().c_str(), &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);
    if (rc != SQLITE_OK) {
        if (error_message) { *error_message = db ? sqlite3_errmsg(db) : "sqlite open failed"; }
        if (db) { sqlite3_close(db); }
        return false;
    }
    sqlite3_close(db);
    return true;
}

bool create_inference_sqlite_table(const std::filesystem::path& db_path, std::string* error_message) {
    sqlite3* db = nullptr;
    const int rc = sqlite3_open_v2(db_path.string().c_str(), &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);
    if (rc != SQLITE_OK) {
        if (error_message) { *error_message = db ? sqlite3_errmsg(db) : "sqlite open failed"; }
        if (db) { sqlite3_close(db); }
        return false;
    }

    const char* sql =
        "CREATE TABLE IF NOT EXISTS t_infer ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "firstName TEXT,"
        "emailAddress TEXT UNIQUE,"
        "amount DECIMAL(10,3),"
        "birthDate DATE,"
        "is_active BOOLEAN,"
        "summary VARCHAR(5),"
        "postal_code TEXT"
        ");";
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
    EXPECT_EQ(invoke_cli({"check"}), cli::exit_codes::kUsage);
}

TEST(CliCommandsTest, DispatchUnknownCommand) {
    EXPECT_EQ(invoke_cli({"unknown-command"}), cli::exit_codes::kOk);
}

TEST(CliCommandsTest, CheckWarningsAndFailures) {
    nlohmann::json warn_root = {
        {"rows", 1},
        {"output", {
            {"type", "database"},
            {"database", nlohmann::json::object()}
        }},
        {"fields", nlohmann::json::array({
            {{"name", "id"}, {"generator", "integer"}, {"config", {{"start", 1}, {"end", 3}}}}
        })}
    };
    const auto warn_path = write_json_file("dg_cli_check_warn.json", warn_root.dump(2));
    EXPECT_EQ(invoke_cli({"check", warn_path.string()}), cli::exit_codes::kOk);

    nlohmann::json bad_regex_root = {
        {"rows", 1},
        {"output", {
            {"type", "file"},
            {"file", {
                {"format", "csv"},
                {"options", {{"header", true}, {"line_ending", "LF"}}}
            }}
        }},
        {"fields", nlohmann::json::array({
            {{"name", "id"}, {"generator", "regular_expression"}, {"config", {{"pattern", "["}}}}
        })}
    };
    const auto bad_regex_path = write_json_file("dg_cli_check_bad_regex.json", bad_regex_root.dump(2));
    EXPECT_EQ(invoke_cli({"check", bad_regex_path.string()}), cli::exit_codes::kRuntimeFailure);

    nlohmann::json bad_db_root = {
        {"rows", 1},
        {"output", {
            {"type", "database"},
            {"database", {
                {"url", "odbc:postgresql:DRIVER={Missing Driver};SERVER=127.0.0.1;PORT=5432;DATABASE=demo;"},
                {"table", "t_data"}
            }}
        }},
        {"fields", nlohmann::json::array({
            {{"name", "id"}, {"generator", "integer"}, {"config", {{"start", 1}, {"end", 3}}}}
        })}
    };
    const auto bad_db_path = write_json_file("dg_cli_check_bad_db.json", bad_db_root.dump(2));
    EXPECT_EQ(invoke_cli({"check", bad_db_path.string()}), cli::exit_codes::kRuntimeFailure);
}

TEST(CliCommandsTest, PreviewFormatsAndDatabasePreview) {
    const auto temp_dir = std::filesystem::temp_directory_path();

    nlohmann::json db_root = {
        {"rows", 1},
        {"output", {
            {"type", "database"},
            {"database", nlohmann::json::object()}
        }},
        {"fields", nlohmann::json::array({
            {{"name", "id"}, {"generator", "integer"}, {"config", {{"start", 1}, {"end", 3}}}}
        })}
    };
    const auto db_path = write_json_file("dg_cli_preview_db.json", db_root.dump(2));
    EXPECT_EQ(invoke_cli({"preview", db_path.string()}), cli::exit_codes::kOk);

    nlohmann::json tab_root = {
        {"rows", 1},
        {"output", {
            {"type", "file"},
            {"file", {
                {"format", "Tab-Delimited"},
                {"options", {{"header", true}, {"line_ending", "CRLF"}}}
            }}
        }},
        {"fields", db_root["fields"]}
    };
    const auto tab_path = write_json_file("dg_cli_preview_tab.json", tab_root.dump(2));
    EXPECT_EQ(invoke_cli({"preview", tab_path.string()}), cli::exit_codes::kOk);

    nlohmann::json custom_root = {
        {"rows", 1},
        {"output", {
            {"type", "file"},
            {"file", {
                {"format", "Custom"},
                {"options", {{"delimiter", "|"}, {"quote", "'"}, {"header", true}, {"line_ending", "LF"}}}
            }}
        }},
        {"fields", db_root["fields"]}
    };
    const auto custom_path = write_json_file("dg_cli_preview_custom.json", custom_root.dump(2));
    EXPECT_EQ(invoke_cli({"preview", custom_path.string()}), cli::exit_codes::kOk);

    nlohmann::json json_root = {
        {"rows", 1},
        {"output", {
            {"type", "file"},
            {"file", {
                {"format", "json"},
                {"options", {{"array", false}, {"include_null", true}}}
            }}
        }},
        {"fields", db_root["fields"]}
    };
    const auto json_path = write_json_file("dg_cli_preview_json.json", json_root.dump(2));
    EXPECT_EQ(invoke_cli({"preview", json_path.string()}), cli::exit_codes::kOk);

    nlohmann::json sql_root = {
        {"rows", 1},
        {"output", {
            {"type", "file"},
            {"file", {
                {"format", "sql"},
                {"options", {{"create_table", false}}}
            }}
        }},
        {"fields", db_root["fields"]}
    };
    const auto sql_path = write_json_file("dg_cli_preview_sql.json", sql_root.dump(2));
    EXPECT_EQ(invoke_cli({"preview", sql_path.string()}), cli::exit_codes::kOk);

    const auto invalid_path = write_json_file("dg_cli_preview_invalid.json", "{");
    EXPECT_EQ(invoke_cli({"preview", invalid_path.string()}), cli::exit_codes::kRuntimeFailure);
}

TEST(CliCommandsTest, InitDatabaseWarningsAndFailures) {
    const auto temp_dir = std::filesystem::temp_directory_path();

    const auto file_db_out = temp_dir / "dg_init_file_db_warn.json";
    EXPECT_EQ(
        invoke_cli({
            "init",
            file_db_out.string(),
            "--template",
            "file",
            "--format",
            "csv",
            "--from-database",
            "sqlite:/tmp/ignored.db",
            "--table",
            "t_data"
        }),
        cli::exit_codes::kOk
    );

    const auto db_missing_table_out = temp_dir / "dg_init_db_missing_table.json";
    EXPECT_EQ(
        invoke_cli({
            "init",
            db_missing_table_out.string(),
            "--template",
            "database",
            "--from-database",
            "sqlite:/tmp/ignored.db"
        }),
        cli::exit_codes::kOk
    );

    const auto invalid_url_out = temp_dir / "dg_init_db_invalid_url.json";
    EXPECT_EQ(
        invoke_cli({
            "init",
            invalid_url_out.string(),
            "--template",
            "database",
            "--from-database",
            "bad",
            "--table",
            "t_data"
        }),
        cli::exit_codes::kUsage
    );

    EXPECT_EQ(
        invoke_cli({
            "init",
            (temp_dir / "dg_init_db_empty_url.json").string(),
            "--template",
            "database",
            "--from-database",
            "",
            "--table",
            "t_data"
        }),
        cli::exit_codes::kUsage
    );

    EXPECT_EQ(
        invoke_cli({
            "init",
            (temp_dir / "dg_init_db_empty_table.json").string(),
            "--template",
            "database",
            "--from-database",
            "sqlite:/tmp/ignored.db",
            "--table",
            ""
        }),
        cli::exit_codes::kUsage
    );

    const auto sqlite_path = temp_dir / "dg_init_missing_table.sqlite";
    std::error_code ec;
    std::filesystem::remove(sqlite_path, ec);
    std::string error;
    ASSERT_TRUE(create_empty_sqlite_db(sqlite_path, &error)) << error;

    const auto missing_table_out = temp_dir / "dg_init_db_missing_table_fail.json";
    EXPECT_EQ(
        invoke_cli({
            "init",
            missing_table_out.string(),
            "--template",
            "database",
            "--from-database",
            std::string("sqlite:") + sqlite_path.string(),
            "--table",
            "missing_table"
        }),
        cli::exit_codes::kRuntimeFailure
    );
}

TEST(CliCommandsTest, InitInfersFieldTemplatesFromSqliteMetadata) {
    const auto temp_dir = std::filesystem::temp_directory_path();
    const auto sqlite_path = temp_dir / "dg_init_infer_rich.sqlite";
    std::error_code ec;
    std::filesystem::remove(sqlite_path, ec);

    std::string error;
    ASSERT_TRUE(create_inference_sqlite_table(sqlite_path, &error)) << error;

    const auto infer_out = temp_dir / "dg_init_infer_rich.json";
    ASSERT_EQ(
        invoke_cli({
            "init",
            infer_out.string(),
            "--template",
            "database",
            "--from-database",
            std::string("sqlite:") + sqlite_path.string(),
            "--table",
            "t_infer"
        }),
        cli::exit_codes::kOk
    );

    const auto inferred_json = read_json_file(infer_out);
    ASSERT_TRUE(inferred_json.contains("fields"));
    const auto& fields = inferred_json["fields"];

    const nlohmann::json* id_field = find_field_by_name(fields, "id");
    ASSERT_NE(id_field, nullptr);
    EXPECT_EQ((*id_field)["generator"], "sequence");
    EXPECT_EQ((*id_field)["config"]["circle"], false);
    EXPECT_FALSE(id_field->contains("unique"));

    const nlohmann::json* first_name_field = find_field_by_name(fields, "firstName");
    ASSERT_NE(first_name_field, nullptr);
    EXPECT_EQ((*first_name_field)["generator"], "first_name");
    EXPECT_TRUE(first_name_field->contains("data_linkage"));
    EXPECT_TRUE(first_name_field->contains("default_value"));
    EXPECT_TRUE(first_name_field->contains("null_value"));

    const nlohmann::json* email_field = find_field_by_name(fields, "emailAddress");
    ASSERT_NE(email_field, nullptr);
    EXPECT_EQ((*email_field)["generator"], "email");
    EXPECT_EQ((*email_field)["unique"], true);

    const nlohmann::json* amount_field = find_field_by_name(fields, "amount");
    ASSERT_NE(amount_field, nullptr);
    EXPECT_EQ((*amount_field)["generator"], "decimal");
    EXPECT_EQ((*amount_field)["config"]["decimal_places"], 3);

    const nlohmann::json* birth_date_field = find_field_by_name(fields, "birthDate");
    ASSERT_NE(birth_date_field, nullptr);
    EXPECT_EQ((*birth_date_field)["generator"], "date");

    const nlohmann::json* active_field = find_field_by_name(fields, "is_active");
    ASSERT_NE(active_field, nullptr);
    EXPECT_EQ((*active_field)["generator"], "enum_item");
    EXPECT_EQ((*active_field)["config"]["enums"], nlohmann::json::array({"1", "0"}));

    const nlohmann::json* summary_field = find_field_by_name(fields, "summary");
    ASSERT_NE(summary_field, nullptr);
    EXPECT_EQ((*summary_field)["generator"], "text");
    EXPECT_EQ((*summary_field)["config"]["number_of_chars_end"], 5);

    const nlohmann::json* postal_code_field = find_field_by_name(fields, "postal_code");
    ASSERT_NE(postal_code_field, nullptr);
    EXPECT_EQ((*postal_code_field)["generator"], "postcode");
}

TEST(CliCommandsTest, SchemaOutputPathFailure) {
    const auto dir_path = std::filesystem::temp_directory_path() / "dg_schema_dir";
    std::error_code ec;
    std::filesystem::create_directories(dir_path, ec);

    EXPECT_EQ(invoke_cli({"schema", dir_path.string()}), cli::exit_codes::kCliError);
}

TEST(CliCommandsTest, SchemaAndDriversRejectInvalidArguments) {
    EXPECT_EQ(invoke_cli({"schema"}), cli::exit_codes::kUsage);
    EXPECT_EQ(invoke_cli({"schema", "--bad"}), cli::exit_codes::kUsage);
    EXPECT_EQ(invoke_cli({"drivers", "--bad"}), cli::exit_codes::kUsage);
}

TEST(CliCommandsTest, RunCoversDatabaseOverrideAndArgumentFailures) {
    const auto temp_dir = std::filesystem::temp_directory_path();
    const auto sqlite_path = temp_dir / "dg_cli_run.sqlite";
    std::error_code ec;
    std::filesystem::remove(sqlite_path, ec);

    std::string error;
    ASSERT_TRUE(create_sqlite_table(sqlite_path, &error)) << error;

    nlohmann::json db_root = {
        {"rows", 2},
        {"output", {
            {"type", "database"},
            {"database", {
                {"url", std::string("sqlite:") + sqlite_path.string()},
                {"table", "t_data"},
                {"insert_mode", "insert"},
                {"batch_size", 2},
                {"queue_size", 2},
                {"threads", 1}
            }}
        }},
        {"fields", nlohmann::json::array({
            {{"name", "id"}, {"generator", "integer"}, {"config", {{"start", 1}, {"end", 3}}}}
        })}
    };
    const auto db_config_path = write_json_file("dg_cli_run_db.json", db_root.dump(2));
    EXPECT_EQ(
        invoke_cli({"run", db_config_path.string(), "--output", (temp_dir / "ignored.csv").string()}),
        cli::exit_codes::kOk
    );

    const auto invalid_input = write_json_file("dg_cli_run_invalid.json", "{");
    EXPECT_EQ(invoke_cli({"run", invalid_input.string()}), cli::exit_codes::kRuntimeFailure);

    const auto valid_input = write_json_file("dg_cli_run_parse_error.json", kValidConfigJson);
    EXPECT_EQ(invoke_cli({"run", valid_input.string(), "--rows", "bad"}), cli::exit_codes::kUsage);
}

TEST(CliCommandsTest, CheckDatabaseConnectionWhenAvailable) {
    const char* pg_url = std::getenv("DATA_GENERATOR_TEST_PG_URL");
    if (!pg_url || std::string(pg_url).empty()) {
        GTEST_SKIP() << "DATA_GENERATOR_TEST_PG_URL not set.";
    }

    nlohmann::json root = {
        {"rows", 1},
        {"output", {
            {"type", "database"},
            {"database", {
                {"url", pg_url},
                {"table", "t_data"}
            }}
        }},
        {"fields", nlohmann::json::array({
            {{"name", "id"}, {"generator", "integer"}, {"config", {{"start", 1}, {"end", 3}}}}
        })}
    };

    const auto path = write_json_file("dg_cli_check_db.json", root.dump(2));
    EXPECT_EQ(invoke_cli({"check", path.string()}), cli::exit_codes::kOk);
}

}  // namespace
