/// @file test_override_rules.cpp

#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include <optional>
#include <string>

#include "generators/override_rules.h"

using namespace data_generator::generator;

namespace {

TEST(OverrideRulesTest, ParseRejectsInvalidPercentAndNullType) {
    EXPECT_THROW(
        (void)parse_overrides(nlohmann::json::parse(R"json({"rows":10,"null_value":{"enabled":true,"percent":101}})json")),
        std::invalid_argument
    );
    EXPECT_THROW(
        (void)parse_overrides(nlohmann::json::parse(R"json({"rows":10,"default_value":{"enabled":true,"percent":-1}})json")),
        std::invalid_argument
    );
    EXPECT_THROW(
        (void)parse_overrides(nlohmann::json::parse(R"json({"rows":10,"null_value_string":1})json")),
        std::invalid_argument
    );
}

TEST(OverrideRulesTest, ParseRejectsCombinedPercentOver100WhenRowsBounded) {
    EXPECT_THROW(
        (void)parse_overrides(nlohmann::json::parse(
            R"json({"rows":10,"null_value":{"enabled":true,"percent":60},"default_value":{"enabled":true,"percent":60,"value":"x"}})json"
        )),
        std::invalid_argument
    );
}

TEST(OverrideRulesTest, NullOverrideWithRowsAlwaysAppliesAt100Percent) {
    auto state = parse_overrides(nlohmann::json::parse(
        R"json({"rows":3,"null_value_string":"<NULL>","null_value":{"enabled":true,"percent":100}})json"
    ));

    std::optional<std::string> v1 = apply_override(state);
    ASSERT_TRUE(v1.has_value());
    EXPECT_EQ(*v1, "<NULL>");
    next_row(state);

    std::optional<std::string> v2 = apply_override(state);
    ASSERT_TRUE(v2.has_value());
    EXPECT_EQ(*v2, "<NULL>");
    next_row(state);

    std::optional<std::string> v3 = apply_override(state);
    ASSERT_TRUE(v3.has_value());
    EXPECT_EQ(*v3, "<NULL>");
}

TEST(OverrideRulesTest, DefaultOverrideWithRowsAlwaysAppliesAt100Percent) {
    auto state = parse_overrides(nlohmann::json::parse(
        R"json({"rows":2,"default_value":{"enabled":true,"percent":100,"value":"DEF"}})json"
    ));

    std::optional<std::string> v1 = apply_override(state);
    ASSERT_TRUE(v1.has_value());
    EXPECT_EQ(*v1, "DEF");
    next_row(state);

    std::optional<std::string> v2 = apply_override(state);
    ASSERT_TRUE(v2.has_value());
    EXPECT_EQ(*v2, "DEF");
}

TEST(OverrideRulesTest, UnboundedModePercentRulesWorkForDeterministicEdges) {
    auto always_null = parse_overrides(nlohmann::json::parse(
        R"json({"null_value":{"enabled":true,"percent":100},"null_value_string":null})json"
    ));
    std::optional<std::string> v1 = apply_override(always_null);
    ASSERT_TRUE(v1.has_value());
    EXPECT_TRUE(v1->empty());

    auto always_default = parse_overrides(nlohmann::json::parse(
        R"json({"default_value":{"enabled":true,"percent":100,"value":"V"}})json"
    ));
    std::optional<std::string> v2 = apply_override(always_default);
    ASSERT_TRUE(v2.has_value());
    EXPECT_EQ(*v2, "V");

    auto none = parse_overrides(nlohmann::json::parse(R"json({})json"));
    std::optional<std::string> v3 = apply_override(none);
    EXPECT_FALSE(v3.has_value());
}

}  // namespace
