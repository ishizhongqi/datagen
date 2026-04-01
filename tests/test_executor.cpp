/// @file test_executor.cpp

#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include <atomic>
#include <mutex>
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

TEST(ExecutorTest, NullOverrideProducesJsonNull) {
    auto cfg = parse_or_fail(R"json(
{
  "rows": 4,
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
      "name": "address_line2",
      "generator": "regular_expression",
      "config": {
        "pattern": "x"
      },
      "null_value": {
        "enabled": true,
        "percentage": 100
      }
    }
  ]
}
)json");

    std::ostringstream out;
    const auto result =
        engine::generate_to_stream(cfg, engine::ExecutionOptions{.requested_threads = 2}, out);

    EXPECT_EQ(result.info.threads_used, 1U);
    EXPECT_TRUE(result.info.fallback_to_single_thread);
    EXPECT_NE(result.info.fallback_reason.find("override"), std::string::npos);

    const auto generated = nlohmann::json::parse(out.str());
    ASSERT_TRUE(generated.is_array());
    ASSERT_EQ(generated.size(), 4U);
    for (const auto& row : generated) {
        ASSERT_TRUE(row.contains("address_line2"));
        ASSERT_TRUE(row.at("address_line2").is_null());
    }
}

TEST(ExecutorTest, IncludeNullFalseOmitsField) {
    auto cfg = parse_or_fail(R"json(
{
  "rows": 2,
  "output": {
    "type": "file",
    "file": {
      "format": "json",
      "options": {
        "array": true,
        "include_null": false
      }
    }
  },
  "fields": [
    {
      "name": "value",
      "generator": "regular_expression",
      "config": {
        "pattern": "x"
      },
      "null_value": {
        "enabled": true,
        "percentage": 100
      }
    }
  ]
}
)json");

    std::ostringstream out;
    (void)engine::generate_to_stream(cfg, engine::ExecutionOptions{.requested_threads = 1}, out);

    const auto generated = nlohmann::json::parse(out.str());
    ASSERT_TRUE(generated.is_array());
    ASSERT_EQ(generated.size(), 2U);
    for (const auto& row : generated) {
        EXPECT_FALSE(row.contains("value"));
    }
}

TEST(ExecutorTest, ParallelIneligibleWorkloadFallsBackToSingleThread) {
    const auto cfg = parse_or_fail(R"json(
{
  "rows": 32,
  "output": {
    "type": "file",
    "file": {
      "format": "csv"
    }
  },
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
  "output": {
    "type": "file",
    "file": {
      "format": "csv"
    }
  },
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

TEST(ExecutorTest, ParallelGenerateWithConsumerCountsGeneratedRows) {
    const auto cfg = parse_or_fail(R"json(
{
  "rows": 24,
  "output": {
    "type": "file",
    "file": {
      "format": "csv"
    }
  },
  "fields": [
    {
      "name": "id",
      "generator": "integer",
      "config": {
        "start": 1,
        "end": 1000
      }
    }
  ]
}
)json");

    std::mutex seen_mutex;
    std::vector<bool> seen(static_cast<std::size_t>(cfg.rows), false);
    std::atomic<bool> saw_missing_value = false;

    const auto result = engine::generate_with_consumer(
        cfg,
        engine::ExecutionOptions{.requested_threads = 4},
        [&](engine::Row&& row, const std::uint64_t row_index) {
            if (row.empty() || !row.front().has_value()) {
                saw_missing_value.store(true);
                return true;
            }

            std::lock_guard lock(seen_mutex);
            seen.at(static_cast<std::size_t>(row_index)) = true;
            return true;
        }
    );

    EXPECT_GE(result.info.threads_used, 2U);
    EXPECT_FALSE(result.info.fallback_to_single_thread);
    EXPECT_EQ(result.rows_generated, static_cast<std::uint64_t>(cfg.rows));
    EXPECT_FALSE(saw_missing_value.load());
    for (const bool row_seen : seen) {
        EXPECT_TRUE(row_seen);
    }
}

TEST(ExecutorTest, ParallelGenerateWithConsumerStopsWhenConsumerCancels) {
    const auto cfg = parse_or_fail(R"json(
{
  "rows": 24,
  "output": {
    "type": "file",
    "file": {
      "format": "csv"
    }
  },
  "fields": [
    {
      "name": "id",
      "generator": "integer",
      "config": {
        "start": 1,
        "end": 1000
      }
    }
  ]
}
)json");

    std::atomic<int> callback_count = 0;

    const auto result = engine::generate_with_consumer(
        cfg,
        engine::ExecutionOptions{.requested_threads = 4},
        [&](engine::Row&&, const std::uint64_t) {
            return callback_count.fetch_add(1) != 0;
        }
    );

    EXPECT_GE(result.info.threads_used, 2U);
    EXPECT_FALSE(result.info.fallback_to_single_thread);
    EXPECT_GE(callback_count.load(), 1);
    EXPECT_LT(result.rows_generated, static_cast<std::uint64_t>(cfg.rows));
}

TEST(ExecutorTest, SequentialGenerateWithConsumerStopsWhenConsumerCancels) {
    const auto cfg = parse_or_fail(R"json(
{
  "rows": 8,
  "output": {
    "type": "file",
    "file": {
      "format": "csv"
    }
  },
  "fields": [
    {
      "name": "id",
      "generator": "integer",
      "config": {
        "start": 1,
        "end": 1000
      }
    }
  ]
}
)json");

    std::atomic<int> callback_count = 0;

    const auto result = engine::generate_with_consumer(
        cfg,
        engine::ExecutionOptions{.requested_threads = 1},
        [&](engine::Row&&, const std::uint64_t) {
            return callback_count.fetch_add(1) != 0;
        }
    );

    EXPECT_EQ(result.info.threads_used, 1U);
    EXPECT_FALSE(result.info.fallback_to_single_thread);
    EXPECT_EQ(callback_count.load(), 1);
    EXPECT_EQ(result.rows_generated, 0U);
}

TEST(ExecutorTest, GenerateToStreamSupportsTabDelimitedCustomAndSqlFormats) {
    auto tab_cfg = parse_or_fail(R"json(
{
  "rows": 2,
  "output": {
    "type": "file",
    "file": {
      "format": "Tab-Delimited",
      "options": {
        "header": true,
        "line_ending": "LF"
      }
    }
  },
  "fields": [
    {"name": "id", "generator": "integer", "config": {"start": 1, "end": 9}},
    {"name": "name", "generator": "regular_expression", "config": {"pattern": "ab"}}
  ]
}
)json");

    std::ostringstream tab_out;
    const auto tab_result =
        engine::generate_to_stream(tab_cfg, engine::ExecutionOptions{.requested_threads = 1}, tab_out);
    EXPECT_EQ(tab_result.rows_generated, 2U);
    EXPECT_NE(tab_out.str().find("id\tname"), std::string::npos);

    auto custom_cfg = parse_or_fail(R"json(
{
  "rows": 2,
  "output": {
    "type": "file",
    "file": {
      "format": "Custom",
      "options": {
        "delimiter": "|",
        "quote": "'",
        "header": true,
        "line_ending": "LF"
      }
    }
  },
  "fields": [
    {"name": "id", "generator": "integer", "config": {"start": 1, "end": 9}},
    {"name": "name", "generator": "regular_expression", "config": {"pattern": "ab"}}
  ]
}
)json");

    std::ostringstream custom_out;
    const auto custom_result =
        engine::generate_to_stream(custom_cfg, engine::ExecutionOptions{.requested_threads = 1}, custom_out);
    EXPECT_EQ(custom_result.rows_generated, 2U);
    EXPECT_NE(custom_out.str().find("id|name"), std::string::npos);

    auto sql_cfg = parse_or_fail(R"json(
{
  "rows": 2,
  "output": {
    "type": "file",
    "file": {
      "format": "sql",
      "options": {
        "table": "t_people",
        "create_table": true
      }
    }
  },
  "fields": [
    {"name": "id", "generator": "integer", "config": {"start": 1, "end": 9}},
    {"name": "name", "generator": "regular_expression", "config": {"pattern": "ab"}}
  ]
}
)json");

    std::ostringstream sql_out;
    const auto sql_result =
        engine::generate_to_stream(sql_cfg, engine::ExecutionOptions{.requested_threads = 1}, sql_out);
    EXPECT_EQ(sql_result.rows_generated, 2U);
    EXPECT_NE(sql_out.str().find("CREATE TABLE IF NOT EXISTS t_people"), std::string::npos);
    EXPECT_NE(sql_out.str().find("INSERT INTO t_people"), std::string::npos);
}

}  // namespace
