/// @file test_run.cpp

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

std::filesystem::path test_input(const std::string& filename) {
    return std::filesystem::path(DATA_GENERATOR_TEST_INPUT_DIR) / filename;
}

TEST(RunTest, HelpCommandReturnsOk) {
    EXPECT_EQ(invoke_cli({"help"}), cli::exit_codes::kOk);
}

TEST(RunTest, GenerateFromInputFileReturnsOkAndWritesOutput) {
    const auto input  = test_input("run_valid.json");
    const auto output = std::filesystem::temp_directory_path() / "data-generator-test-output.csv";

    std::error_code ec;
    std::filesystem::remove(output, ec);

    const int rc = invoke_cli({
        "generate",
        "--input",
        input.string(),
        "--output",
        output.string(),
        "--rows",
        "16",
        "--threads",
        "2",
    });

    EXPECT_EQ(rc, cli::exit_codes::kOk);
    EXPECT_TRUE(std::filesystem::exists(output));

    std::ifstream generated(output);
    std::string data;
    std::getline(generated, data);
    EXPECT_FALSE(data.empty());

    std::filesystem::remove(output, ec);
}

TEST(RunTest, ValidateMalformedJsonReturnsRuntimeFailure) {
    const auto input = test_input("malformed.json");
    EXPECT_EQ(
        invoke_cli({"validate", "--input", input.string()}),
        cli::exit_codes::kRuntimeFailure
    );
}

TEST(RunTest, ValidateMissingFieldsReturnsRuntimeFailure) {
    const auto input = test_input("missing_fields.json");
    EXPECT_EQ(
        invoke_cli({"validate", "--input", input.string(), "--require-output"}),
        cli::exit_codes::kRuntimeFailure
    );
}

}  // namespace
