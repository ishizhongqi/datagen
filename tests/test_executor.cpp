/// @file test_executor.cpp

#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include <sstream>
#include <string>
#include <vector>

#include "config/configuration.h"
#include "engine/executor.h"

using namespace data_generator;

namespace {

config::GenerationConfig parse_or_fail(const char* json_text) {
    const auto root = nlohmann::json::parse(json_text);

    config::GenerationConfig cfg;
    std::vector<config::ValidationIssue> issues;
    const bool ok = config::parse_generation_config(root, config::ParseMode::RequireOutputSettings, &cfg, &issues);
    EXPECT_TRUE(ok);
    EXPECT_TRUE(issues.empty());
    return cfg;
}

TEST(ExecutorTest, ReplacesEmptyWithGlobalNullLiteral) {
    auto cfg = parse_or_fail(R"json(
{
  "rows": 4,
  "destination": "file",
  "file_format": "json",
  "null_value_string": "N/A",
  "fields": [
    {
      "name": "address_line2",
      "generator": "regular_expression",
      "config": {
        "pattern": ""
      }
    }
  ]
}
)json");

    std::ostringstream out;
    const auto result =
        engine::generate_to_stream(cfg, engine::ExecutionOptions{.requested_threads = 2}, out);

    EXPECT_EQ(result.info.threads_used, 2U);
    EXPECT_FALSE(result.info.fallback_to_single_thread);

    const auto generated = nlohmann::json::parse(out.str());
    ASSERT_TRUE(generated.is_array());
    ASSERT_EQ(generated.size(), 4U);
    for (const auto& row : generated) {
      ASSERT_TRUE(row.contains("address_line2"));
      ASSERT_TRUE(row.at("address_line2").is_string());
      EXPECT_EQ(row.at("address_line2").get<std::string>(), "N/A");
    }
}

TEST(ExecutorTest, ParallelIneligibleWorkloadFallsBackToSingleThread) {
    const auto cfg = parse_or_fail(R"json(
{
  "rows": 32,
  "destination": "file",
  "file_format": "csv",
  "fields": [
    {
      "name": "seq",
      "generator": "sequence",
      "config": {
        "start": 1,
        "end": 100,
        "step": 1,
        "circle": true
      }
    }
  ]
}
)json");

    std::ostringstream out;
    const auto result =
        engine::generate_to_stream(cfg, engine::ExecutionOptions{.requested_threads = 4}, out);

    EXPECT_EQ(result.info.threads_used, 1U);
    EXPECT_TRUE(result.info.fallback_to_single_thread);
    EXPECT_FALSE(result.info.fallback_reason.empty());
    EXPECT_FALSE(out.str().empty());
}

TEST(ExecutorTest, GeneratesWithoutSeedOption) {
    const auto cfg = parse_or_fail(R"json(
{
  "rows": 2,
  "destination": "file",
  "file_format": "csv",
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
}
)json");

    std::ostringstream out;
    EXPECT_NO_THROW((void)engine::generate_to_stream(cfg, engine::ExecutionOptions{.requested_threads = 1}, out));
    EXPECT_FALSE(out.str().empty());
}

}  // namespace
