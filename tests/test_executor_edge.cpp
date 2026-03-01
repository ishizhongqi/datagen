/// @file test_executor_edge.cpp

#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include <sstream>

#include "core/configuration.h"
#include "core/executor.h"

using namespace data_generator::core;

namespace {

TEST(ExecutorEdgeTest, ShouldRenderNullPolicyBranches) {
    NullPolicy p1;
    p1.configured = true;
    p1.null_if_empty = true;
    EXPECT_TRUE(should_render_null(p1, ""));
    EXPECT_FALSE(should_render_null(p1, "x"));

    NullPolicy p2;
    p2.configured = false;
    p2.null_if_empty = true;
    EXPECT_FALSE(should_render_null(p2, ""));
}

TEST(ExecutorEdgeTest, PreviewRowAndNullLiteralBranches) {
    const auto root = nlohmann::json::parse(R"json(
{
  "rows": 1,
  "output_format": "json",
  "null_value_string": "N",
  "fields": [
    {"name":"f","generator":"regular_expression","config":{"pattern":""}}
  ]
}
)json");

    GenerationConfig cfg;
    std::vector<ValidationIssue> issues;
    ASSERT_TRUE(parse_generation_config(root, ParseMode::RequireOutputSettings, &cfg, &issues));

    auto row = preview_row(cfg);
    ASSERT_EQ(row.size(), 1U);
    ASSERT_TRUE(row[0].has_value());
    EXPECT_EQ(*row[0], "N");
}

TEST(ExecutorEdgeTest, GenerateSingleThreadPath) {
    const auto root = nlohmann::json::parse(R"json(
{
  "rows": 1,
  "output_format": "csv",
  "fields": [
    {"name":"f","generator":"integer","config":{"start":1,"end":9}}
  ]
}
)json");

    GenerationConfig cfg;
    std::vector<ValidationIssue> issues;
    ASSERT_TRUE(parse_generation_config(root, ParseMode::RequireOutputSettings, &cfg, &issues));

    std::ostringstream out;
    const auto result = generate_to_stream(cfg, ExecutionOptions{.requested_threads = 8}, out);
    EXPECT_EQ(result.info.threads_used, 1U);
}

TEST(ExecutorEdgeTest, ParallelFallbackBecauseOfOverrideRule) {
    const auto root = nlohmann::json::parse(R"json(
{
  "rows": 16,
  "output_format": "json",
  "fields": [
    {
      "name":"v",
      "generator":"integer",
      "config":{"start":1,"end":9},
      "null_value":{"enabled":true,"percent":10}
    }
  ]
}
)json");

    GenerationConfig cfg;
    std::vector<ValidationIssue> issues;
    ASSERT_TRUE(parse_generation_config(root, ParseMode::RequireOutputSettings, &cfg, &issues));

    std::ostringstream out;
    const auto result =
        generate_to_stream(cfg, ExecutionOptions{.requested_threads = 4}, out);
    EXPECT_TRUE(result.info.fallback_to_single_thread);
    EXPECT_NE(result.info.fallback_reason.find("override"), std::string::npos);
}

TEST(ExecutorEdgeTest, NullIfEmptyProducesJsonNullAndSqlPathWorks) {
    const auto root = nlohmann::json::parse(R"json(
{
  "rows": 2,
  "output_format": "json",
  "null_value_string": null,
  "fields": [
    {"name":"f","generator":"regular_expression","config":{"pattern":""}}
  ]
}
)json");

    GenerationConfig cfg;
    std::vector<ValidationIssue> issues;
    ASSERT_TRUE(parse_generation_config(root, ParseMode::RequireOutputSettings, &cfg, &issues));

    std::ostringstream out_json;
    (void)generate_to_stream(cfg, ExecutionOptions{.requested_threads = 2}, out_json);
    EXPECT_NE(out_json.str().find("null"), std::string::npos);

    cfg.format = OutputFormat::Sql;
    std::ostringstream out_sql;
    (void)generate_to_stream(cfg, ExecutionOptions{.requested_threads = 1}, out_sql);
    EXPECT_NE(out_sql.str().find("INSERT INTO"), std::string::npos);
}

TEST(ExecutorEdgeTest, NormalizeEmptyValuesUsesNullLiteralWhenNullIfEmptyDisabled) {
    GenerationConfig cfg;
    cfg.rows   = 1;
    cfg.format = OutputFormat::Json;
    cfg.null_policy.configured = true;
    cfg.null_policy.null_if_empty = false;
    cfg.null_policy.null_literal  = std::string("N");
    FieldSpec field;
    field.name         = "f";
    field.generator    = "regular_expression";
    field.raw          = nlohmann::json::parse(R"json({"config":{"pattern":""}})json");
    field.data_linkage = std::nullopt;
    field.unique       = false;
    cfg.fields.push_back(field);

    std::ostringstream out;
    (void)generate_to_stream(cfg, ExecutionOptions{.requested_threads = 1}, out);
    EXPECT_NE(out.str().find("\"N\""), std::string::npos);
}

TEST(ExecutorEdgeTest, ParallelFallbackReasonsForUniqueAndLinkage) {
    const auto root_base = nlohmann::json::parse(R"json(
{
  "rows": 16,
  "output_format": "csv",
  "fields": [
    {"name":"u","generator":"integer","config":{"start":1,"end":100}}
  ]
}
)json");
    GenerationConfig cfg_base;
    std::vector<ValidationIssue> issues;
    ASSERT_TRUE(parse_generation_config(root_base, ParseMode::RequireOutputSettings, &cfg_base, &issues));

    auto cfg_unique = cfg_base;
    cfg_unique.fields[0].unique = true;
    std::ostringstream out1;
    const auto res1 =
        generate_to_stream(cfg_unique, ExecutionOptions{.requested_threads = 4}, out1);
    EXPECT_TRUE(res1.info.fallback_to_single_thread);
    EXPECT_NE(res1.info.fallback_reason.find("unique"), std::string::npos);

    auto cfg_linkage = cfg_base;
    cfg_linkage.fields[0].data_linkage = std::string("num:g");
    std::ostringstream out2;
    const auto res2 =
        generate_to_stream(cfg_linkage, ExecutionOptions{.requested_threads = 4}, out2);
    EXPECT_TRUE(res2.info.fallback_to_single_thread);
    EXPECT_NE(res2.info.fallback_reason.find("data_linkage"), std::string::npos);
}

TEST(ExecutorEdgeTest, ParallelWorkerExceptionPath) {
    const auto root = nlohmann::json::parse(R"json(
{
  "rows": 8,
  "output_format": "json",
  "fields": [
    {"name":"bad","generator":"regular_expression","config":{"pattern":"["}}
  ]
}
)json");
    GenerationConfig cfg;
    std::vector<ValidationIssue> issues;
    ASSERT_TRUE(parse_generation_config(root, ParseMode::RequireOutputSettings, &cfg, &issues));
    std::ostringstream out;
    EXPECT_THROW(
        (void)generate_to_stream(cfg, ExecutionOptions{.requested_threads = 4}, out),
        std::invalid_argument
    );
}

}  // namespace
