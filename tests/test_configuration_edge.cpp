/// @file test_configuration_edge.cpp

#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include <vector>

#include "config/configuration.h"

using namespace data_generator;

namespace {

TEST(ConfigurationEdgeTest, RootAndOutputValidationBranches) {
    config::GenerationConfig cfg;
    std::vector<config::ValidationIssue> issues;

    EXPECT_FALSE(config::parse_generation_config(
        nlohmann::json::array(),
        config::ParseMode::RequireOutputSettings,
        &cfg,
        &issues
    ));
    EXPECT_FALSE(issues.empty());

    EXPECT_FALSE(config::parse_generation_config(
        nlohmann::json::parse(R"json({"rows":"x","output":1,"fields":[]})json"),
        config::ParseMode::RequireOutputSettings,
        &cfg,
        &issues
    ));
    EXPECT_FALSE(issues.empty());

    EXPECT_FALSE(config::parse_generation_config(
        nlohmann::json::parse(R"json({"rows":1,"output":{"type":"bad"},"fields":[{"name":"f","generator":"integer","config":{"start":1,"end":2}}]})json"),
        config::ParseMode::RequireOutputSettings,
        &cfg,
        &issues
    ));

    EXPECT_FALSE(config::parse_generation_config(
        nlohmann::json::parse(R"json({"rows":1,"output":{"type":"file","file":{"format":"bad"}},"fields":[{"name":"f","generator":"integer","config":{"start":1,"end":2}}]})json"),
        config::ParseMode::RequireOutputSettings,
        &cfg,
        &issues
    ));

    EXPECT_FALSE(config::parse_generation_config(
        nlohmann::json::parse(
            R"json({"rows":1,"output":{"type":"file","file":{"format":"csv"},"unknown":1},"unknown_root":1,"fields":[{"name":"f","generator":"integer","config":{"start":1,"end":2},"unknown_field":1}]})json"
        ),
        config::ParseMode::RequireOutputSettings,
        &cfg,
        &issues
    ));

    EXPECT_TRUE(config::parse_generation_config(
        nlohmann::json::parse(
            R"json({"$schema":"./schema/data-generator.schema.json","rows":1,"output":{"type":"file","file":{"format":"csv"}},"fields":[{"name":"f","generator":"integer","config":{"start":1,"end":2}}]})json"
        ),
        config::ParseMode::RequireOutputSettings,
        &cfg,
        &issues
    ));

    EXPECT_FALSE(config::parse_generation_config(
        nlohmann::json::parse(R"json({"output":{"type":"file","file":{"format":"csv"}},"fields":[{"name":"f","generator":"integer","config":{"start":1,"end":2}}]})json"),
        config::ParseMode::RequireOutputSettings,
        &cfg,
        &issues
    ));
    EXPECT_FALSE(config::parse_generation_config(
        nlohmann::json::parse(R"json({"rows":0,"output":{"type":"file","file":{"format":"csv"}},"fields":[{"name":"f","generator":"integer","config":{"start":1,"end":2}}]})json"),
        config::ParseMode::RequireOutputSettings,
        &cfg,
        &issues
    ));
    EXPECT_FALSE(config::parse_generation_config(
        nlohmann::json::parse(R"json({"rows":1,"fields":[{"name":"f","generator":"integer","config":{"start":1,"end":2}}]})json"),
        config::ParseMode::RequireOutputSettings,
        &cfg,
        &issues
    ));
    EXPECT_FALSE(config::parse_generation_config(
        nlohmann::json::parse(R"json({"rows":1,"output":{"type":"file","file":{"format":"csv"}},"fields":{}})json"),
        config::ParseMode::RequireOutputSettings,
        &cfg,
        &issues
    ));

    EXPECT_FALSE(config::parse_generation_config(
        nlohmann::json::parse(R"json({"rows":1,"output":{"type":"file","file":{"format":"csv"}},"fields":[{"name":"f","generator":"date","unique":true,"config":{"start_date":"2024-01-01","end_date":"2024-01-31"}}]})json"),
        config::ParseMode::RequireOutputSettings,
        &cfg,
        &issues
    ));
}

TEST(ConfigurationEdgeTest, FieldValidationBranches) {
    config::GenerationConfig cfg;
    std::vector<config::ValidationIssue> issues;

    EXPECT_FALSE(config::parse_generation_config(
        nlohmann::json::parse(R"json({"rows":1,"output":{"type":"file","file":{"format":"csv"}},"fields":[1]})json"),
        config::ParseMode::RequireOutputSettings,
        &cfg,
        &issues
    ));

    EXPECT_FALSE(config::parse_generation_config(
        nlohmann::json::parse(
            R"json({"rows":1,"output":{"type":"file","file":{"format":"csv"}},"fields":[{"name":1,"generator":2,"config":3}]})json"
        ),
        config::ParseMode::RequireOutputSettings,
        &cfg,
        &issues
    ));

    EXPECT_FALSE(config::parse_generation_config(
        nlohmann::json::parse(
            R"json({"rows":1,"output":{"type":"file","file":{"format":"csv"}},"fields":[{"name":"f","generator":"integer","config":{"start":1,"end":2},"unique":"x"}]})json"
        ),
        config::ParseMode::RequireOutputSettings,
        &cfg,
        &issues
    ));

    EXPECT_FALSE(config::parse_generation_config(
        nlohmann::json::parse(
            R"json({"rows":1,"output":{"type":"file","file":{"format":"csv"}},"fields":[{"name":"f","generator":"integer","config":{"start":1,"end":2},"data_linkage":123}]})json"
        ),
        config::ParseMode::RequireOutputSettings,
        &cfg,
        &issues
    ));

    EXPECT_FALSE(config::parse_generation_config(
        nlohmann::json::parse(
            R"json({"rows":1,"output":{"type":"file","file":{"format":"csv"}},"fields":[{"name":"f","generator":"integer","config":{"start":1,"end":2,"bad":1},"default_value":1}]})json"
        ),
        config::ParseMode::RequireOutputSettings,
        &cfg,
        &issues
    ));

    EXPECT_TRUE(config::parse_generation_config(
        nlohmann::json::parse(
            R"json({"rows":1,"output":{"type":"file","file":{"format":"csv"}},"fields":[{"name":"f","generator":"date","config":{"end_date":"2024-01-01"}}]})json"
        ),
        config::ParseMode::RequireOutputSettings,
        &cfg,
        &issues
    ));
}

TEST(ConfigurationEdgeTest, AllowMissingOutputModeSucceedsWithDefaults) {
    config::GenerationConfig cfg;
    std::vector<config::ValidationIssue> issues;
    EXPECT_TRUE(config::parse_generation_config(
        nlohmann::json::parse(
            R"json({"fields":[{"name":"f","generator":"integer","config":{"start":1,"end":2}}]})json"
        ),
        config::ParseMode::AllowMissingOutputSettings,
        &cfg,
        &issues
    ));
    EXPECT_EQ(cfg.rows, 100);
    EXPECT_EQ(cfg.output.type, config::OutputType::File);
    EXPECT_EQ(cfg.output.file.format, config::OutputFormat::Csv);
}

TEST(ConfigurationEdgeTest, ApplyOverridesAndFormatRoundtrip) {
    EXPECT_EQ(config::parse_output_format("csv"), config::OutputFormat::Csv);
    EXPECT_EQ(config::parse_output_format("json"), config::OutputFormat::Json);
    EXPECT_EQ(config::parse_output_format("sql"), config::OutputFormat::Sql);
    EXPECT_EQ(config::parse_output_format("Tab-Delimited"), config::OutputFormat::TabDelimited);
    EXPECT_EQ(config::parse_output_format("Custom"), config::OutputFormat::Custom);
    EXPECT_FALSE(config::parse_output_format("x").has_value());

    EXPECT_EQ(config::output_format_to_string(config::OutputFormat::Csv), "csv");
    EXPECT_EQ(config::output_format_to_string(config::OutputFormat::Json), "json");
    EXPECT_EQ(config::output_format_to_string(config::OutputFormat::Sql), "sql");
    EXPECT_EQ(config::output_format_to_string(config::OutputFormat::TabDelimited), "Tab-Delimited");
    EXPECT_EQ(config::output_format_to_string(config::OutputFormat::Custom), "Custom");
    EXPECT_EQ(config::output_format_to_string(static_cast<config::OutputFormat>(99)), "csv");

    EXPECT_EQ(config::parse_output_type("file"), config::OutputType::File);
    EXPECT_EQ(config::parse_output_type("database"), config::OutputType::Database);
    EXPECT_FALSE(config::parse_output_type("x").has_value());

    config::GenerationConfig cfg;
    cfg.rows = 10;
    cfg.output.file.format = config::OutputFormat::Csv;
    cfg.output.database.table = "t";

    config::CliOverrides overrides;
    overrides.rows = 20;
    overrides.format = config::OutputFormat::Sql;
    overrides.table_name = std::string("t2");
    overrides.type = config::OutputType::Database;
    overrides.output_path = std::string("out.csv");
    apply_cli_overrides(&cfg, overrides);
    EXPECT_EQ(cfg.rows, 20);
    EXPECT_EQ(cfg.output.file.format, config::OutputFormat::Sql);
    EXPECT_EQ(cfg.output.database.table, "t2");
    EXPECT_EQ(cfg.output.file.sql.table, "t2");
    EXPECT_EQ(cfg.output.type, config::OutputType::Database);
    EXPECT_EQ(cfg.output.file.path, "out.csv");
}

}  // namespace
