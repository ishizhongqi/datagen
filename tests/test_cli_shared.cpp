/// @file test_cli_shared.cpp

#include <gtest/gtest.h>

#include <cxxopts.hpp>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

#include "cli/cli_shared.h"

using namespace data_generator::cli;

namespace {

TEST(CliSharedTest, LoadJsonAndParseOptionsAndValidationOutput) {
    const auto good = std::filesystem::temp_directory_path() / "dg_cli_shared_good.json";
    {
        std::ofstream out(good, std::ios::trunc);
        out << "{\"a\":1}";
    }
    const auto j = load_json_from_file(good.string());
    EXPECT_TRUE(j.contains("a"));

    const auto bad = std::filesystem::temp_directory_path() / "dg_cli_shared_bad.json";
    {
        std::ofstream out(bad, std::ios::trunc);
        out << "{";
    }
    EXPECT_THROW((void)load_json_from_file(bad.string()), std::runtime_error);
    EXPECT_THROW((void)load_json_from_file("/tmp/no_such_file_1234567.json"), std::runtime_error);

    cxxopts::Options options("prog", "desc");
    options.add_options()("x", "x", cxxopts::value<int>());
    const auto parsed = parse_options(options, {"-x", "3"});
    EXPECT_EQ(parsed["x"].as<int>(), 3);

    std::ostringstream oss;
    print_validation_issues({{"$.a", "bad"}, {"$.b", "missing"}}, oss);
    EXPECT_NE(oss.str().find("$.a"), std::string::npos);
    EXPECT_NE(oss.str().find("$.b"), std::string::npos);
}

TEST(CliSharedTest, BuildDescribeLinesBranches) {
    GeneratorMetadata meta;
    meta.name = "g";
    meta.module = "m";
    meta.supports_unique = true;
    meta.supports_data_linkage = true;
    meta.config_template = Json::object();
    meta.config_template["a_bool"] = true;
    meta.config_template["a_array"] = Json::array({1, 2});
    meta.config_template["a_str"] = "x";
    meta.config_params.push_back(ConfigParam{"a_bool", "boolean", "desc", false, {}});
    meta.config_params.push_back(ConfigParam{"a_array", "", "desc", false, {}});
    meta.config_params.push_back(ConfigParam{"a_str", "string", "desc", false, {"x", "y"}});

    const auto lines = build_describe_text_lines(meta);
    EXPECT_FALSE(lines.empty());
}

}  // namespace
