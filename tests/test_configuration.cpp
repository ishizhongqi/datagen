/// @file test_configuration.cpp

#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include <string>
#include <vector>

#include "config/configuration.h"

using namespace data_generator;

namespace {

TEST(ConfigurationTest, ParseValidConfig) {
    const auto root = nlohmann::json::parse(R"json(
{
  "rows": 5,
  "output": {
    "type": "file",
    "file": {
      "format": "json",
      "options": {
        "array": true,
        "include_null": true
      }
    }
  },
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

    config::GenerationConfig cfg;
    std::vector<config::ValidationIssue> issues;
    const bool ok = config::parse_generation_config(root, config::ParseMode::RequireOutputSettings, &cfg, &issues);

    ASSERT_TRUE(ok);
    EXPECT_TRUE(issues.empty());
    EXPECT_EQ(cfg.rows, 5);
    EXPECT_EQ(cfg.output.type, config::OutputType::File);
    EXPECT_EQ(cfg.output.file.format, config::OutputFormat::Json);
    EXPECT_TRUE(cfg.output.file.json.array);
    EXPECT_TRUE(cfg.output.file.json.include_null);
    ASSERT_EQ(cfg.fields.size(), 1U);
    EXPECT_EQ(cfg.fields.front().name, "id");
}

TEST(ConfigurationTest, MissingFieldsReturnsError) {
    const auto root = nlohmann::json::parse(R"json(
{
  "rows": 5,
  "output": {
    "type": "file",
    "file": {
      "format": "csv"
    }
  }
}
)json");

    config::GenerationConfig cfg;
    std::vector<config::ValidationIssue> issues;
    const bool ok = config::parse_generation_config(root, config::ParseMode::RequireOutputSettings, &cfg, &issues);

    EXPECT_FALSE(ok);
    EXPECT_FALSE(issues.empty());
}

TEST(ConfigurationTest, UnknownGeneratorReturnsError) {
    const auto root = nlohmann::json::parse(R"json(
{
  "rows": 3,
  "output": {
    "type": "file",
    "file": {
      "format": "csv"
    }
  },
  "fields": [
    {
      "name": "f1",
      "generator": "not_exists",
      "config": {}
    }
  ]
}
)json");

    config::GenerationConfig cfg;
    std::vector<config::ValidationIssue> issues;
    const bool ok = config::parse_generation_config(root, config::ParseMode::RequireOutputSettings, &cfg, &issues);

    EXPECT_FALSE(ok);
    ASSERT_FALSE(issues.empty());
    EXPECT_NE(issues.front().message.find("unknown generator"), std::string::npos);
}

TEST(ConfigurationTest, ParseCustomFileOptions) {
    const auto root = nlohmann::json::parse(R"json(
{
  "rows": 2,
  "output": {
    "type": "file",
    "file": {
      "format": "Custom",
      "options": {
        "delimiter": "|",
        "quote": "'",
        "header": false,
        "line_ending": "CRLF"
      }
    }
  },
  "fields": [
    {
      "name": "id",
      "generator": "integer",
      "config": {
        "start": 1,
        "end": 2
      }
    }
  ]
}
)json");

    config::GenerationConfig cfg;
    std::vector<config::ValidationIssue> issues;
    const bool ok = config::parse_generation_config(root, config::ParseMode::RequireOutputSettings, &cfg, &issues);

    ASSERT_TRUE(ok);
    EXPECT_TRUE(issues.empty());
    EXPECT_EQ(cfg.output.type, config::OutputType::File);
    EXPECT_EQ(cfg.output.file.format, config::OutputFormat::Custom);
    EXPECT_EQ(cfg.output.file.custom.delimiter, "|");
    EXPECT_EQ(cfg.output.file.custom.quote, "'");
    EXPECT_FALSE(cfg.output.file.custom.header);
    EXPECT_EQ(cfg.output.file.custom.line_ending, config::LineEnding::CRLF);
}

}  // namespace
