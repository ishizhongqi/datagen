/// @file test_configuration_edge.cpp

#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include <vector>

#include "core/configuration.h"

using namespace data_generator::core;

namespace {

TEST(ConfigurationEdgeTest, RootAndOutputValidationBranches) {
    GenerationConfig cfg;
    std::vector<ValidationIssue> issues;

    EXPECT_FALSE(parse_generation_config(nlohmann::json::array(), ParseMode::RequireOutputSettings, &cfg, &issues));
    EXPECT_FALSE(issues.empty());

    EXPECT_FALSE(parse_generation_config(
        nlohmann::json::parse(R"json({"rows":"x","output_format":1,"fields":[]})json"),
        ParseMode::RequireOutputSettings,
        &cfg,
        &issues
    ));
    EXPECT_FALSE(issues.empty());

    EXPECT_FALSE(parse_generation_config(
        nlohmann::json::parse(R"json({"rows":1,"output_format":"bad","fields":[]})json"),
        ParseMode::RequireOutputSettings,
        &cfg,
        &issues
    ));

    EXPECT_FALSE(parse_generation_config(
        nlohmann::json::parse(
            R"json({"rows":1,"output_format":"csv","unknown_root":1,"fields":[{"name":"f","generator":"integer","config":{"start":1,"end":2},"unknown_field":1}]})json"
        ),
        ParseMode::RequireOutputSettings,
        &cfg,
        &issues
    ));

    EXPECT_TRUE(parse_generation_config(
        nlohmann::json::parse(
            R"json({"$schema":"./schema/data-generator.schema.json","rows":1,"output_format":"csv","fields":[{"name":"f","generator":"integer","config":{"start":1,"end":2}}]})json"
        ),
        ParseMode::RequireOutputSettings,
        &cfg,
        &issues
    ));

    EXPECT_FALSE(parse_generation_config(
        nlohmann::json::parse(R"json({"output_format":"csv","fields":[{"name":"f","generator":"integer","config":{"start":1,"end":2}}]})json"),
        ParseMode::RequireOutputSettings,
        &cfg,
        &issues
    ));
    EXPECT_FALSE(parse_generation_config(
        nlohmann::json::parse(R"json({"rows":0,"output_format":"csv","fields":[{"name":"f","generator":"integer","config":{"start":1,"end":2}}]})json"),
        ParseMode::RequireOutputSettings,
        &cfg,
        &issues
    ));
    EXPECT_FALSE(parse_generation_config(
        nlohmann::json::parse(R"json({"rows":1,"fields":[{"name":"f","generator":"integer","config":{"start":1,"end":2}}]})json"),
        ParseMode::RequireOutputSettings,
        &cfg,
        &issues
    ));
    EXPECT_FALSE(parse_generation_config(
        nlohmann::json::parse(R"json({"rows":1,"output_format":"csv","fields":{}})json"),
        ParseMode::RequireOutputSettings,
        &cfg,
        &issues
    ));

    EXPECT_FALSE(parse_generation_config(
        nlohmann::json::parse(R"json({"rows":1,"output_format":"csv","fields":[{"name":"f","generator":"date","unique":true,"config":{"start_date":"2024-01-01","end_date":"2024-01-31"}}]})json"),
        ParseMode::RequireOutputSettings,
        &cfg,
        &issues
    ));
}

TEST(ConfigurationEdgeTest, FieldValidationBranches) {
    GenerationConfig cfg;
    std::vector<ValidationIssue> issues;

    EXPECT_FALSE(parse_generation_config(
        nlohmann::json::parse(R"json({"rows":1,"output_format":"csv","fields":[1]})json"),
        ParseMode::RequireOutputSettings,
        &cfg,
        &issues
    ));

    EXPECT_FALSE(parse_generation_config(
        nlohmann::json::parse(
            R"json({"rows":1,"output_format":"csv","fields":[{"name":1,"generator":2,"config":3}]})json"
        ),
        ParseMode::RequireOutputSettings,
        &cfg,
        &issues
    ));

    EXPECT_FALSE(parse_generation_config(
        nlohmann::json::parse(
            R"json({"rows":1,"output_format":"csv","fields":[{"name":"f","generator":"integer","config":{"start":1,"end":2},"unique":"x"}]})json"
        ),
        ParseMode::RequireOutputSettings,
        &cfg,
        &issues
    ));

    EXPECT_FALSE(parse_generation_config(
        nlohmann::json::parse(
            R"json({"rows":1,"output_format":"csv","fields":[{"name":"f","generator":"integer","config":{"start":1,"end":2},"data_linkage":123}]})json"
        ),
        ParseMode::RequireOutputSettings,
        &cfg,
        &issues
    ));

    EXPECT_FALSE(parse_generation_config(
        nlohmann::json::parse(
            R"json({"rows":1,"output_format":"csv","table_name":"","include_create_table":"x","null_value_string":1,"fields":[{"name":"f","generator":"integer","config":{"start":1,"end":2},"null_value":1,"default_value":1}]})json"
        ),
        ParseMode::RequireOutputSettings,
        &cfg,
        &issues
    ));

    EXPECT_TRUE(parse_generation_config(
        nlohmann::json::parse(
            R"json({"rows":1,"output_format":"csv","fields":[{"name":"f","generator":"date","config":{"end_date":"2024-01-01"}}]})json"
        ),
        ParseMode::RequireOutputSettings,
        &cfg,
        &issues
    ));
}

TEST(ConfigurationEdgeTest, AllowMissingOutputModeSucceedsWithDefaults) {
    GenerationConfig cfg;
    std::vector<ValidationIssue> issues;
    EXPECT_TRUE(parse_generation_config(
        nlohmann::json::parse(
            R"json({"fields":[{"name":"f","generator":"integer","config":{"start":1,"end":2}}]})json"
        ),
        ParseMode::AllowMissingOutputSettings,
        &cfg,
        &issues
    ));
    EXPECT_EQ(cfg.rows, 100);
    EXPECT_EQ(cfg.format, OutputFormat::Csv);

    EXPECT_TRUE(parse_generation_config(
        nlohmann::json::parse(
            R"json({"null_value_string":null,"fields":[{"name":"f","generator":"integer","config":{"start":1,"end":2}}]})json"
        ),
        ParseMode::AllowMissingOutputSettings,
        &cfg,
        &issues
    ));
    EXPECT_TRUE(cfg.null_policy.configured);
    EXPECT_TRUE(cfg.null_policy.null_if_empty);
}

TEST(ConfigurationEdgeTest, ApplyOverridesAndFormatRoundtrip) {
    EXPECT_EQ(parse_output_format("csv"), OutputFormat::Csv);
    EXPECT_EQ(parse_output_format("json"), OutputFormat::Json);
    EXPECT_EQ(parse_output_format("sql"), OutputFormat::Sql);
    EXPECT_FALSE(parse_output_format("x").has_value());

    EXPECT_EQ(output_format_to_string(OutputFormat::Csv), "csv");
    EXPECT_EQ(output_format_to_string(OutputFormat::Json), "json");
    EXPECT_EQ(output_format_to_string(OutputFormat::Sql), "sql");
    EXPECT_EQ(output_format_to_string(static_cast<OutputFormat>(99)), "csv");

    GenerationConfig cfg;
    cfg.rows = 10;
    cfg.format = OutputFormat::Csv;
    cfg.table_name = "t";
    cfg.include_create_table = true;

    apply_cli_overrides(&cfg, 20, OutputFormat::Sql, std::string("t2"), false);
    EXPECT_EQ(cfg.rows, 20);
    EXPECT_EQ(cfg.format, OutputFormat::Sql);
    EXPECT_EQ(cfg.table_name, "t2");
    EXPECT_FALSE(cfg.include_create_table);
}

}  // namespace
