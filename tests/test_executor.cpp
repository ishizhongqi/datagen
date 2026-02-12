/// @file test_executor.cpp

#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include <sstream>
#include <string>
#include <vector>

#include "core/configuration.h"
#include "core/executor.h"

using namespace data_generator::core;

namespace {

GenerationConfig parse_or_fail(const char* json_text) {
    const auto root = nlohmann::json::parse(json_text);

    GenerationConfig cfg;
    std::vector<ValidationIssue> issues;
    const bool ok = parse_generation_config(root, ParseMode::RequireOutputSettings, &cfg, &issues);
    EXPECT_TRUE(ok);
    EXPECT_TRUE(issues.empty());
    return cfg;
}

TEST(ExecutorTest, ReplacesEmptyWithGlobalNullLiteral) {
    auto cfg = parse_or_fail(R"json(
{
  "rows": 4,
  "output_format": "json",
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
        generate_to_stream(cfg, ExecutionOptions{.requested_threads = 2, .seed = std::nullopt}, out);

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
  "output_format": "csv",
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
        generate_to_stream(cfg, ExecutionOptions{.requested_threads = 4, .seed = std::nullopt}, out);

    EXPECT_EQ(result.info.threads_used, 1U);
    EXPECT_TRUE(result.info.fallback_to_single_thread);
    EXPECT_FALSE(result.info.fallback_reason.empty());
    EXPECT_FALSE(out.str().empty());
}

TEST(ExecutorTest, ThrowsWhenSeedIsRequested) {
    const auto cfg = parse_or_fail(R"json(
{
  "rows": 2,
  "output_format": "csv",
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
    EXPECT_THROW(
        (void)generate_to_stream(cfg, ExecutionOptions{.requested_threads = 1, .seed = 42U}, out),
        std::runtime_error
    );
}

}  // namespace
