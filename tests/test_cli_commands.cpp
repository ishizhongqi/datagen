/// @file test_cli_commands.cpp

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

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

TEST(CliCommandsTest, InitDescribeListPreviewAndValidateBranches) {
    EXPECT_EQ(invoke_cli({"list"}), cli::exit_codes::kOk);
    EXPECT_EQ(invoke_cli({"list", "--help"}), cli::exit_codes::kOk);

    EXPECT_EQ(invoke_cli({"describe", "--generator", "integer"}), cli::exit_codes::kOk);
    EXPECT_EQ(invoke_cli({"describe", "--generator", "integer", "--json"}), cli::exit_codes::kOk);
    EXPECT_EQ(invoke_cli({"describe", "--generator", "unsigned_integer"}), cli::exit_codes::kUsage);
    EXPECT_EQ(invoke_cli({"describe", "--generator", "decimal"}), cli::exit_codes::kOk);
    EXPECT_EQ(invoke_cli({"describe", "--generator", "decimal_string"}), cli::exit_codes::kUsage);
    EXPECT_EQ(invoke_cli({"describe", "--generator", "not_exist"}), cli::exit_codes::kUsage);
    EXPECT_EQ(invoke_cli({"describe"}), cli::exit_codes::kUsage);

    EXPECT_EQ(invoke_cli({"init"}), cli::exit_codes::kOk);
    EXPECT_EQ(invoke_cli({"init", "--generator", "integer", "--rows", "5", "--format", "sql"}), cli::exit_codes::kOk);
    EXPECT_EQ(invoke_cli({"init", "--rows", "5", "--format", "sql"}), cli::exit_codes::kOk);
    EXPECT_EQ(invoke_cli({"init", "--generator", "company_name"}), cli::exit_codes::kOk);
    EXPECT_EQ(invoke_cli({"init", "--generator", "unsigned_integer"}), cli::exit_codes::kUsage);
    EXPECT_EQ(invoke_cli({"init", "--generator", "decimal"}), cli::exit_codes::kOk);
    EXPECT_EQ(invoke_cli({"init", "--generator", "decimal_string"}), cli::exit_codes::kUsage);
    EXPECT_EQ(invoke_cli({"init", "--rows", "0"}), cli::exit_codes::kUsage);
    EXPECT_EQ(invoke_cli({"init", "--format", "bad"}), cli::exit_codes::kUsage);
    EXPECT_EQ(invoke_cli({"init", "--generator", "not_exist"}), cli::exit_codes::kUsage);

    const auto init_out = std::filesystem::temp_directory_path() / "dg_cli_init_template.json";
    EXPECT_EQ(
        invoke_cli({"init", "--generator", "integer", "--output", init_out.string()}),
        cli::exit_codes::kOk
    );
    EXPECT_TRUE(std::filesystem::exists(init_out));

    const auto valid_input = write_json_file(
        "dg_cli_valid_preview.json",
        R"json({"rows":2,"output_format":"csv","fields":[{"name":"id","generator":"integer","config":{"start":1,"end":9}}]})json"
    );

    EXPECT_EQ(invoke_cli({"preview", "--input", valid_input.string(), "--field", "all", "--format", "csv"}), cli::exit_codes::kOk);
    EXPECT_EQ(invoke_cli({"preview", "--input", valid_input.string(), "--field", "all", "--format", "json"}), cli::exit_codes::kOk);
    EXPECT_EQ(invoke_cli({"preview", "--input", valid_input.string(), "--field", "id", "--format", "json"}), cli::exit_codes::kOk);
    EXPECT_EQ(invoke_cli({"preview", "--input", valid_input.string(), "--format", "sql"}), cli::exit_codes::kOk);
    EXPECT_EQ(invoke_cli({"preview", "--input", valid_input.string(), "--field", "missing"}), cli::exit_codes::kRuntimeFailure);
    EXPECT_EQ(invoke_cli({"preview", "--input", valid_input.string(), "--format", "bad"}), cli::exit_codes::kUsage);
    EXPECT_EQ(invoke_cli({"preview"}), cli::exit_codes::kUsage);

    EXPECT_EQ(invoke_cli({"validate", "--input", valid_input.string()}), cli::exit_codes::kOk);
    EXPECT_EQ(invoke_cli({"validate", "--input", valid_input.string(), "--require-output"}), cli::exit_codes::kOk);
    EXPECT_EQ(invoke_cli({"schema"}), cli::exit_codes::kOk);
    EXPECT_EQ(invoke_cli({"schema", "--help"}), cli::exit_codes::kOk);

    const auto invalid_input = write_json_file("dg_cli_invalid.json", "{");
    EXPECT_EQ(invoke_cli({"validate", "--input", invalid_input.string()}), cli::exit_codes::kRuntimeFailure);

    const auto invalid_schema = write_json_file("dg_cli_invalid_schema.json", R"json({"rows":2,"output_format":"csv","fields":[]})json");
    EXPECT_EQ(invoke_cli({"preview", "--input", invalid_schema.string()}), cli::exit_codes::kRuntimeFailure);
}

TEST(CliCommandsTest, GenerateErrorAndSuccessBranches) {
    const auto valid_input = write_json_file(
        "dg_cli_valid_generate.json",
        R"json({"rows":2,"output_format":"csv","fields":[{"name":"id","generator":"integer","config":{"start":1,"end":9}}]})json"
    );

    EXPECT_EQ(invoke_cli({"generate"}), cli::exit_codes::kUsage);
    EXPECT_EQ(invoke_cli({"generate", "--input", valid_input.string(), "--rows", "0"}), cli::exit_codes::kUsage);
    EXPECT_EQ(invoke_cli({"generate", "--input", valid_input.string(), "--format", "bad"}), cli::exit_codes::kUsage);
    EXPECT_EQ(invoke_cli({"generate", "--input", valid_input.string(), "--table", ""}), cli::exit_codes::kUsage);
    EXPECT_EQ(
        invoke_cli({"generate", "--input", valid_input.string(), "--create-table", "--no-create-table"}),
        cli::exit_codes::kUsage
    );
    EXPECT_EQ(invoke_cli({"generate", "--input", valid_input.string(), "--threads", "0"}), cli::exit_codes::kUsage);

    const auto out_csv = std::filesystem::temp_directory_path() / "dg_cli_out.csv";
    EXPECT_EQ(
        invoke_cli({
            "generate",
            "--input",
            valid_input.string(),
            "--output",
            out_csv.string(),
            "--rows",
            "10",
            "--format",
            "json",
            "--table",
            "t_cli",
            "--no-create-table",
            "--threads",
            "2",
        }),
        cli::exit_codes::kOk
    );

    EXPECT_TRUE(std::filesystem::exists(out_csv));

    const auto bad_path = std::filesystem::temp_directory_path() / "no_such_dir" / "x.csv";
    EXPECT_EQ(
        invoke_cli({"generate", "--input", valid_input.string(), "--output", bad_path.string()}),
        cli::exit_codes::kRuntimeFailure
    );

    const auto fallback_input = write_json_file(
        "dg_cli_generate_fallback.json",
        R"json({"rows":2,"output_format":"csv","fields":[{"name":"email","generator":"email","unique":true,"config":{"languages":["English"]}}]})json"
    );
    EXPECT_EQ(
        invoke_cli({"generate", "--input", fallback_input.string(), "--threads", "4"}),
        cli::exit_codes::kOk
    );

    const auto invalid_schema = write_json_file("dg_cli_generate_invalid_schema.json", R"json({"rows":2,"output_format":"csv","fields":[]})json");
    EXPECT_EQ(invoke_cli({"generate", "--input", invalid_schema.string()}), cli::exit_codes::kRuntimeFailure);

    EXPECT_EQ(
        invoke_cli({"generate", "--input", "/tmp/no_such_input_987654321.json"}),
        cli::exit_codes::kRuntimeFailure
    );
}

TEST(CliCommandsTest, DispatchUnknownCommand) {
    EXPECT_EQ(invoke_cli({"unknown-command"}), cli::exit_codes::kOk);
}

TEST(CliCommandsTest, HelpAndParseErrorBranches) {
    EXPECT_EQ(invoke_cli({"generate", "--help"}), cli::exit_codes::kOk);
    EXPECT_EQ(invoke_cli({"preview", "--help"}), cli::exit_codes::kOk);
    EXPECT_EQ(invoke_cli({"validate", "--help"}), cli::exit_codes::kOk);
    EXPECT_EQ(invoke_cli({"describe", "--help"}), cli::exit_codes::kOk);
    EXPECT_EQ(invoke_cli({"init", "--help"}), cli::exit_codes::kOk);
    EXPECT_EQ(invoke_cli({"schema", "--help"}), cli::exit_codes::kOk);

    EXPECT_EQ(invoke_cli({"generate", "--bad-opt"}), cli::exit_codes::kUsage);
    EXPECT_EQ(invoke_cli({"preview", "--bad-opt"}), cli::exit_codes::kUsage);
    EXPECT_EQ(invoke_cli({"validate", "--bad-opt"}), cli::exit_codes::kUsage);
    EXPECT_EQ(invoke_cli({"describe", "--bad-opt"}), cli::exit_codes::kUsage);
    EXPECT_EQ(invoke_cli({"init", "--bad-opt"}), cli::exit_codes::kUsage);
    EXPECT_EQ(invoke_cli({"list", "--bad-opt"}), cli::exit_codes::kUsage);
    EXPECT_EQ(invoke_cli({"schema", "--bad-opt"}), cli::exit_codes::kUsage);

    const auto bad_output = std::filesystem::temp_directory_path() / "no_such_dir_2" / "init.json";
    EXPECT_EQ(
        invoke_cli({"init", "--generator", "integer", "--output", bad_output.string()}),
        cli::exit_codes::kCliError
    );

    EXPECT_EQ(invoke_cli({"validate"}), cli::exit_codes::kUsage);

    const auto schema_out = std::filesystem::temp_directory_path() / "dg_cli_schema.json";
    EXPECT_EQ(invoke_cli({"schema", "--output", schema_out.string()}), cli::exit_codes::kOk);
    EXPECT_TRUE(std::filesystem::exists(schema_out));

    const auto bad_schema_output = std::filesystem::temp_directory_path() / "no_such_dir_schema" / "schema.json";
    EXPECT_EQ(invoke_cli({"schema", "--output", bad_schema_output.string()}), cli::exit_codes::kCliError);
}

TEST(CliCommandsTest, PreviewAndValidateRuntimeErrorBranches) {
    const auto null_preview = write_json_file(
        "dg_cli_preview_null.json",
        R"json({
  "rows": 1,
  "null_value_string": null,
  "output_format": "csv",
  "fields": [
    {
      "name": "value",
      "generator": "regular_expression",
      "config": {"pattern": ""}
    }
  ]
})json"
    );
    EXPECT_EQ(
        invoke_cli({"preview", "--input", null_preview.string(), "--field", "value"}),
        cli::exit_codes::kOk
    );

    EXPECT_EQ(
        invoke_cli({"preview", "--input", "/tmp/no_such_preview_987654321.json"}),
        cli::exit_codes::kRuntimeFailure
    );

    const auto runtime_invalid = write_json_file(
        "dg_cli_validate_runtime_invalid.json",
        R"json({
  "rows": 1,
  "output_format": "csv",
  "fields": [
    {
      "name": "bad",
      "generator": "regular_expression",
      "config": {"pattern": "["}
    }
  ]
})json"
    );
    EXPECT_EQ(
        invoke_cli({"validate", "--input", runtime_invalid.string(), "--require-output"}),
        cli::exit_codes::kRuntimeFailure
    );
}

}  // namespace
