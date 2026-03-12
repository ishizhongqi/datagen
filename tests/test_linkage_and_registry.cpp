/// @file test_linkage_and_registry.cpp

#include <gtest/gtest.h>

#include <memory>
#include <optional>
#include <string>

#include "generators/core/generator_base.h"
#include "generators/core/generator_registry.h"
#include "generators/core/linkage_helper.h"

using namespace data_generator::generator;

namespace {

class DummyGenerator : public IGenerator {
public:
    std::string generate() override { return "dummy"; }
    void next() override {}
};

class DefaultNextGenerator : public IGenerator {
public:
    std::string generate() override { return "x"; }
};

TEST(LinkageHelperTest, ParsesBooleanAndStringForms) {
    EXPECT_FALSE(parse_linkage_key(Json::object()).has_value());
    EXPECT_FALSE(parse_linkage_key(Json::parse(R"json({"data_linkage":false})json")).has_value());

    const auto default_key = parse_linkage_key(Json::parse(R"json({"data_linkage":true})json"));
    ASSERT_TRUE(default_key.has_value());
    EXPECT_EQ(*default_key, "default");

    const auto custom_key = parse_linkage_key(Json::parse(R"json({"data_linkage":"mod:group"})json"));
    ASSERT_TRUE(custom_key.has_value());
    EXPECT_EQ(*custom_key, "mod:group");

    EXPECT_FALSE(parse_linkage_key(Json::parse(R"json({"data_linkage":""})json")).has_value());
    EXPECT_THROW((void)parse_linkage_key(Json::parse(R"json({"data_linkage":123})json")), std::invalid_argument);
}

TEST(GeneratorRegistryTest, RegisterAndCreateAndUnknown) {
    GeneratorRegistry registry;
    registry.register_generator("dummy", [](const Json&) {
        return std::make_unique<DummyGenerator>();
    });

    auto g = registry.create("dummy", Json::object());
    ASSERT_NE(g, nullptr);
    EXPECT_EQ(g->generate(), "dummy");

    EXPECT_THROW((void)registry.create("missing", Json::object()), std::runtime_error);
}

TEST(GeneratorRegistryTest, BaseDefaultNextIsCallable) {
    DefaultNextGenerator g;
    EXPECT_EQ(g.generate(), "x");
    EXPECT_NO_THROW(g.next());
}

}  // namespace
