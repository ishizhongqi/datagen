// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file test_configuration.cpp

#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include <string>
#include <vector>

#include "core/configuration.h"

namespace data_generator::core {
namespace {

TEST(ConfigurationTest, ParseValidConfig) {
    const auto root = nlohmann::json::parse(R"json(
{
  "rows": 5,
  "output_format": "json",
  "null_value_string": "<NULL>",
  "table_name": "t_orders",
  "include_create_table": false,
  "fields": [
    {
      "name": "id",
      "generator": "integer",
      "config": {
        "start": 1,
        "end": 100
      }
    }
  ]
}
)json");

    GenerationConfig cfg;
    std::vector<ValidationIssue> issues;
    const bool ok = parse_generation_config(root, ParseMode::RequireOutputSettings, &cfg, &issues);

    ASSERT_TRUE(ok);
    EXPECT_TRUE(issues.empty());
    EXPECT_EQ(cfg.rows, 5);
    EXPECT_EQ(cfg.format, OutputFormat::Json);
    EXPECT_EQ(cfg.table_name, "t_orders");
    EXPECT_FALSE(cfg.include_create_table);
    ASSERT_EQ(cfg.fields.size(), 1U);
    EXPECT_EQ(cfg.fields.front().name, "id");
    ASSERT_TRUE(cfg.null_policy.configured);
    ASSERT_TRUE(cfg.null_policy.null_literal.has_value());
    EXPECT_EQ(*cfg.null_policy.null_literal, "<NULL>");
}

TEST(ConfigurationTest, MissingFieldsReturnsError) {
    const auto root = nlohmann::json::parse(R"json(
{
  "rows": 5,
  "output_format": "csv"
}
)json");

    GenerationConfig cfg;
    std::vector<ValidationIssue> issues;
    const bool ok = parse_generation_config(root, ParseMode::RequireOutputSettings, &cfg, &issues);

    EXPECT_FALSE(ok);
    EXPECT_FALSE(issues.empty());
}

TEST(ConfigurationTest, UnknownGeneratorReturnsError) {
    const auto root = nlohmann::json::parse(R"json(
{
  "rows": 3,
  "output_format": "csv",
  "fields": [
    {
      "name": "f1",
      "generator": "not_exists",
      "config": {}
    }
  ]
}
)json");

    GenerationConfig cfg;
    std::vector<ValidationIssue> issues;
    const bool ok = parse_generation_config(root, ParseMode::RequireOutputSettings, &cfg, &issues);

    EXPECT_FALSE(ok);
    ASSERT_FALSE(issues.empty());
    EXPECT_NE(issues.front().message.find("unknown generator"), std::string::npos);
}

}  // namespace
}  // namespace data_generator::core
