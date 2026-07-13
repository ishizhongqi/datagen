/// @file test_utility_generators.cpp

#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include <sstream>
#include <string>

#include "config/configuration.h"
#include "engine/executor.h"

using namespace datagen;

namespace {

config::GenerationConfig cfg_from(const nlohmann::json& root) {
    config::GenerationConfig cfg;
    std::vector<config::ValidationIssue> issues;
    const bool ok = config::parse_generation_config(root, config::ParseMode::RequireOutputSettings, &cfg, &issues);
    EXPECT_TRUE(ok);
    return cfg;
}

TEST(UtilityGeneratorsTest, SequenceAndRegexVariants) {
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
    {"name":"seq_circle","generator":"sequence","config":{"start":1,"end":2,"step":1,"circle":true}},
    {"name":"seq_stop","generator":"sequence","config":{"start":1,"end":2,"step":1,"circle":false}},
    {"name":"r1","generator":"regular_expression","config":{"pattern":"[A-Z]{2}[0-9]{2}"}},
    {"name":"r2","generator":"regular_expression","config":{"pattern":"\\d+"}},
    {"name":"r3","generator":"regular_expression","config":{"pattern":"[a-z]?x*"}},
    {"name":"r4","generator":"regular_expression","config":{"pattern":"[^A]{1,2}"}},
    {"name":"r5","generator":"regular_expression","config":{"pattern":"a{2,4}"}}
  ]
}
)json");

    auto cfg = cfg_from(root);
    std::ostringstream out;
    (void)engine::generate_to_stream(cfg, engine::ExecutionOptions{.requested_threads = 1}, out);
    EXPECT_NE(out.str().find("seq_circle"), std::string::npos);
}

TEST(UtilityGeneratorsTest, BooleanGeneratorUsesNativeBooleanInJsonAndTextInCsv) {
    const auto json_root = nlohmann::json::parse(R"json(
{
  "rows": 1,
  "output": {
    "type": "file",
    "file": {
      "format": "json"
    }
  },
  "fields": [
    {"name":"flag","generator":"boolean","config":{"true_percentage":100.0}}
  ]
}
)json");

    auto json_cfg = cfg_from(json_root);
    std::ostringstream json_out;
    (void)engine::generate_to_stream(json_cfg, engine::ExecutionOptions{.requested_threads = 1}, json_out);
    const auto parsed = nlohmann::json::parse(json_out.str());
    ASSERT_TRUE(parsed.is_array());
    ASSERT_EQ(parsed.size(), 1u);
    ASSERT_TRUE(parsed[0]["flag"].is_boolean());
    EXPECT_TRUE(parsed[0]["flag"].get<bool>());

    const auto csv_root = nlohmann::json::parse(R"json(
{
  "rows": 1,
  "output": {
    "type": "file",
    "file": {
      "format": "csv"
    }
  },
  "fields": [
    {"name":"flag","generator":"boolean","config":{"true_percentage":0.0}}
  ]
}
)json");

    auto csv_cfg = cfg_from(csv_root);
    std::ostringstream csv_out;
    (void)engine::generate_to_stream(csv_cfg, engine::ExecutionOptions{.requested_threads = 1}, csv_out);
    EXPECT_NE(csv_out.str().find("false"), std::string::npos);
}

TEST(UtilityGeneratorsTest, BooleanGeneratorRejectsOutOfRangeProbability) {
    const auto root = nlohmann::json::parse(R"json(
{
  "rows": 1,
  "output": {
    "type": "file",
    "file": {
      "format": "json"
    }
  },
  "fields": [
    {"name":"flag","generator":"boolean","config":{"true_percentage":150.0}}
  ]
}
)json");

    auto cfg = cfg_from(root);
    std::ostringstream out;
    EXPECT_THROW(
        (void)engine::generate_to_stream(cfg, engine::ExecutionOptions{.requested_threads = 1}, out),
        std::invalid_argument
    );
}

TEST(UtilityGeneratorsTest, RegexInvalidPatternsThrow) {
    const auto root = nlohmann::json::parse(R"json(
{
  "rows": 1,
  "output": {
    "type": "file",
    "file": {
      "format": "json"
    }
  },
  "fields": [
    {"name":"bad","generator":"regular_expression","config":{"pattern":"[abc"}}
  ]
}
)json");

    auto cfg = cfg_from(root);
    std::ostringstream out;
    EXPECT_THROW(
        (void)engine::generate_to_stream(cfg, engine::ExecutionOptions{.requested_threads = 1}, out),
        std::invalid_argument
    );
}

TEST(UtilityGeneratorsTest, RegexMoreInvalidPatternsThrow) {
    const auto root1 = nlohmann::json::parse(R"json(
{
  "rows": 1,
  "output": {
    "type": "file",
    "file": {
      "format": "json"
    }
  },
  "fields": [
    {"name":"bad","generator":"regular_expression","config":{"pattern":"+"}}
  ]
}
)json");
    auto cfg1 = cfg_from(root1);
    std::ostringstream out1;
    EXPECT_THROW(
        (void)engine::generate_to_stream(cfg1, engine::ExecutionOptions{.requested_threads = 1}, out1),
        std::invalid_argument
    );

    const auto root2 = nlohmann::json::parse(R"json(
{
  "rows": 1,
  "output": {
    "type": "file",
    "file": {
      "format": "json"
    }
  },
  "fields": [
    {"name":"bad","generator":"regular_expression","config":{"pattern":"\\\\{"}}
  ]
}
)json");
    auto cfg2 = cfg_from(root2);
    std::ostringstream out2;
    EXPECT_THROW(
        (void)engine::generate_to_stream(cfg2, engine::ExecutionOptions{.requested_threads = 1}, out2),
        std::invalid_argument
    );

    const auto root3 = nlohmann::json::parse(R"json(
{
  "rows": 1,
  "output": {
    "type": "file",
    "file": {
      "format": "json"
    }
  },
  "fields": [
    {"name":"bad","generator":"regular_expression","config":{"pattern":"a{3,1}"}}
  ]
}
)json");
    auto cfg3 = cfg_from(root3);
    std::ostringstream out3;
    EXPECT_THROW(
        (void)engine::generate_to_stream(cfg3, engine::ExecutionOptions{.requested_threads = 1}, out3),
        std::invalid_argument
    );

    const auto root4 = nlohmann::json::parse(R"json(
{
  "rows": 1,
  "output": {
    "type": "file",
    "file": {
      "format": "json"
    }
  },
  "fields": [
    {"name":"ok","generator":"regular_expression","config":{"pattern":"\\\\w+"}}
  ]
}
)json");
    auto cfg4 = cfg_from(root4);
    std::ostringstream out4;
    EXPECT_NO_THROW(
        (void)engine::generate_to_stream(cfg4, engine::ExecutionOptions{.requested_threads = 1}, out4)
    );

    const auto root5 = nlohmann::json::parse(R"json(
{
  "rows": 1,
  "output": {
    "type": "file",
    "file": {
      "format": "json"
    }
  },
  "fields": [
    {"name":"ok","generator":"regular_expression","config":{"pattern":"a{2,}"}}
  ]
}
)json");
    auto cfg5 = cfg_from(root5);
    std::ostringstream out5;
    EXPECT_NO_THROW(
        (void)engine::generate_to_stream(cfg5, engine::ExecutionOptions{.requested_threads = 1}, out5)
    );

    const auto root5b = nlohmann::json::parse(R"json(
{
  "rows": 1,
  "output": {
    "type": "file",
    "file": {
      "format": "json"
    }
  },
  "fields": [
    {"name":"ok","generator":"regular_expression","config":{"pattern":"[\\w]{3}"}}
  ]
}
)json");
    auto cfg5b = cfg_from(root5b);
    std::ostringstream out5b;
    EXPECT_NO_THROW(
        (void)engine::generate_to_stream(cfg5b, engine::ExecutionOptions{.requested_threads = 1}, out5b)
    );

    const auto root6 = nlohmann::json::parse(R"json(
{
  "rows": 1,
  "output": {
    "type": "file",
    "file": {
      "format": "json"
    }
  },
  "fields": [
    {"name":"bad","generator":"regular_expression","config":{"pattern":"a{2,3"}}
  ]
}
)json");
    auto cfg6 = cfg_from(root6);
    std::ostringstream out6;
    EXPECT_THROW(
        (void)engine::generate_to_stream(cfg6, engine::ExecutionOptions{.requested_threads = 1}, out6),
        std::invalid_argument
    );
}

}  // namespace
