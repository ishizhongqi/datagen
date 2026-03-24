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
    const auto missing = std::filesystem::temp_directory_path() / "dg_cli_shared_missing_1234567.json";
    EXPECT_THROW((void)load_json_from_file(missing.string()), std::runtime_error);

    cxxopts::Options options("prog", "desc");
    options.add_options()("x", "x", cxxopts::value<int>());
    const auto parsed = parse_options(options, {"-x", "3"});
    EXPECT_EQ(parsed["x"].as<int>(), 3);

    std::ostringstream oss;
    print_validation_issues(
        {
            {false, "$.a", "bad"},
            {true, "$.b", "missing"},
        },
        oss
    );
    EXPECT_NE(oss.str().find("$.a"), std::string::npos);
    EXPECT_NE(oss.str().find("$.b"), std::string::npos);
}

TEST(CliSharedTest, OrderedConfigTemplateUsesParamOrderAndPreservesExtraKeys) {
    data_generator::config::GeneratorMetadata meta;
    meta.config_template = Json::object();
    meta.config_template["z_last"] = 9;
    meta.config_template["flag"] = true;
    meta.config_template["count"] = 5;
    meta.config_params.push_back(data_generator::config::ConfigParam{"flag", "boolean", "desc", false, {}});
    meta.config_params.push_back(data_generator::config::ConfigParam{"count", "number", "desc", false, {}});

    const OrderedJson ordered = build_ordered_config_template(meta);
    auto it = ordered.begin();
    ASSERT_NE(it, ordered.end());
    EXPECT_EQ(it.key(), "flag");
    ++it;
    ASSERT_NE(it, ordered.end());
    EXPECT_EQ(it.key(), "count");
    ++it;
    ASSERT_NE(it, ordered.end());
    EXPECT_EQ(it.key(), "z_last");
}

TEST(CliSharedTest, RequiredFlagsOnlyUseTranslationCanBeFalse) {
    for (const auto& meta : data_generator::config::get_generator_catalog()) {
        for (const auto& param : meta.config_params) {
            if (param.name == "use_translation") {
                EXPECT_FALSE(param.required);
            } else {
                EXPECT_TRUE(param.required) << "generator=" << meta.name << " param=" << param.name;
            }
        }
    }
}

}  // namespace
