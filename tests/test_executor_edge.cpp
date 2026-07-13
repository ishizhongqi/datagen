/// @file test_executor_edge.cpp

#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include <sstream>

#include "config/configuration.h"
#include "engine/executor.h"

using namespace datagen;

namespace {

TEST(ExecutorEdgeTest, PreviewRowMatchesFieldCount) {
    const auto root = nlohmann::json::parse(R"json(
{
  "rows": 1,
  "output": {
    "type": "file",
    "file": {
      "format": "csv"
    }
  },
  "fields": [
    {"name":"f1","generator":"integer","config":{"start":1,"end":2}},
    {"name":"f2","generator":"integer","config":{"start":1,"end":2}}
  ]
}
)json");

    config::GenerationConfig cfg;
    std::vector<config::ValidationIssue> issues;
    ASSERT_TRUE(config::parse_generation_config(root, config::ParseMode::RequireOutputSettings, &cfg, &issues));

    auto row = engine::preview_row(cfg);
    ASSERT_EQ(row.size(), 2U);
}

TEST(ExecutorEdgeTest, GenerateSingleThreadPath) {
    const auto root = nlohmann::json::parse(R"json(
{
  "rows": 1,
  "output": {
    "type": "file",
    "file": {
      "format": "csv"
    }
  },
  "fields": [
    {"name":"f","generator":"integer","config":{"start":1,"end":9}}
  ]
}
)json");

    config::GenerationConfig cfg;
    std::vector<config::ValidationIssue> issues;
    ASSERT_TRUE(config::parse_generation_config(root, config::ParseMode::RequireOutputSettings, &cfg, &issues));

    std::ostringstream out;
    const auto result = engine::generate_to_stream(cfg, engine::ExecutionOptions{.requested_threads = 8}, out);
    EXPECT_EQ(result.info.threads_used, 1U);
}

TEST(ExecutorEdgeTest, ParallelFallbackBecauseOfOverrideRule) {
    const auto root = nlohmann::json::parse(R"json(
{
  "rows": 16,
  "output": {
    "type": "file",
    "file": {
      "format": "json"
    }
  },
  "fields": [
    {
      "name":"v",
      "generator":"integer",
      "config":{"start":1,"end":9},
      "null_value":{"enabled":true,"percentage":10}
    }
  ]
}
)json");

    config::GenerationConfig cfg;
    std::vector<config::ValidationIssue> issues;
    ASSERT_TRUE(config::parse_generation_config(root, config::ParseMode::RequireOutputSettings, &cfg, &issues));

    std::ostringstream out;
    const auto result =
        engine::generate_to_stream(cfg, engine::ExecutionOptions{.requested_threads = 4}, out);
    EXPECT_TRUE(result.info.fallback_to_single_thread);
    EXPECT_NE(result.info.fallback_reason.find("override"), std::string::npos);
}

TEST(ExecutorEdgeTest, ParallelFallbackReasonsForUniqueAndLinkage) {
    const auto root_base = nlohmann::json::parse(R"json(
{
  "rows": 16,
  "output": {
    "type": "file",
    "file": {
      "format": "csv"
    }
  },
  "fields": [
    {"name":"u","generator":"integer","config":{"start":1,"end":100}}
  ]
}
)json");
    config::GenerationConfig cfg_base;
    std::vector<config::ValidationIssue> issues;
    ASSERT_TRUE(config::parse_generation_config(root_base, config::ParseMode::RequireOutputSettings, &cfg_base, &issues));

    auto cfg_unique = cfg_base;
    cfg_unique.fields[0].unique = true;
    std::ostringstream out1;
    const auto res1 =
        engine::generate_to_stream(cfg_unique, engine::ExecutionOptions{.requested_threads = 4}, out1);
    EXPECT_TRUE(res1.info.fallback_to_single_thread);
    EXPECT_NE(res1.info.fallback_reason.find("unique"), std::string::npos);

    auto cfg_linkage = cfg_base;
    cfg_linkage.fields[0].group = std::string("num:g");
    std::ostringstream out2;
    const auto res2 =
        engine::generate_to_stream(cfg_linkage, engine::ExecutionOptions{.requested_threads = 4}, out2);
    EXPECT_TRUE(res2.info.fallback_to_single_thread);
    EXPECT_NE(res2.info.fallback_reason.find("group"), std::string::npos);
}

TEST(ExecutorEdgeTest, ParallelWorkerExceptionPath) {
    const auto root = nlohmann::json::parse(R"json(
{
  "rows": 8,
  "output": {
    "type": "file",
    "file": {
      "format": "json"
    }
  },
  "fields": [
    {"name":"bad","generator":"regular_expression","config":{"pattern":"["}}
  ]
}
)json");
    config::GenerationConfig cfg;
    std::vector<config::ValidationIssue> issues;
    ASSERT_TRUE(config::parse_generation_config(root, config::ParseMode::RequireOutputSettings, &cfg, &issues));
    std::ostringstream out;
    EXPECT_THROW(
        (void)engine::generate_to_stream(cfg, engine::ExecutionOptions{.requested_threads = 4}, out),
        std::invalid_argument
    );
}

}  // namespace
