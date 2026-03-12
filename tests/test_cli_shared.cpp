/// @file test_cli_shared.cpp

#include <gtest/gtest.h>

#include <cxxopts.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

#include "cli/cli_shared.h"

using namespace data_generator::cli;

namespace {

const Json* find_generator_constraint(const Json& all_of, const std::string& generator_name) {
    for (const auto& item : all_of) {
        if (!item.contains("if")) { continue; }
        const Json& if_schema = item.at("if");
        if (!if_schema.contains("properties")) { continue; }
        const Json& properties = if_schema.at("properties");
        if (!properties.contains("generator")) { continue; }
        const Json& generator = properties.at("generator");
        if (!generator.contains("const") || !generator.at("const").is_string()) { continue; }
        if (generator.at("const").get<std::string>() == generator_name) { return &item; }
    }
    return nullptr;
}

bool contains_not_required_key(const Json& all_of, const std::string& key_name) {
    for (const auto& item : all_of) {
        if (!item.contains("not")) { continue; }
        const Json& not_schema = item.at("not");
        if (!not_schema.contains("required") || !not_schema.at("required").is_array()) { continue; }
        if (not_schema.at("required").size() != 1 || !not_schema.at("required").at(0).is_string()) { continue; }
        if (not_schema.at("required").at(0).get<std::string>() == key_name) { return true; }
    }
    return false;
}

TEST(CliSharedTest, LoadJsonAndParseOptionsAndValidationOutput) {
    const auto good = std::filesystem::temp_directory_path() / "dg_cli_shared_good.json";
    {
        std::ofstream out(good, std::ios::trunc);
        out << "{\"a\":1}";
    }
    const auto j = load_json_from_file(good.string());
    EXPECT_TRUE(j.contains("a"));

    const auto bad = std::filesystem::temp_directory_path() / "dg_cli_shared_bad.json";
    {
        std::ofstream out(bad, std::ios::trunc);
        out << "{";
    }
    EXPECT_THROW((void)load_json_from_file(bad.string()), std::runtime_error);
    const auto missing = std::filesystem::temp_directory_path() / "dg_cli_shared_missing_1234567.json";
    EXPECT_THROW((void)load_json_from_file(missing.string()), std::runtime_error);

    cxxopts::Options options("prog", "desc");
    options.add_options()("x", "x", cxxopts::value<int>());
    const auto parsed = parse_options(options, {"-x", "3"});
    EXPECT_EQ(parsed["x"].as<int>(), 3);

    std::ostringstream oss;
    print_validation_issues(
        {
            {false, "$.a", "bad"},
            {true, "$.b", "missing"},
        },
        oss
    );
    EXPECT_NE(oss.str().find("$.a"), std::string::npos);
    EXPECT_NE(oss.str().find("$.b"), std::string::npos);
}

TEST(CliSharedTest, BuildDescribeLinesBranches) {
    data_generator::config::GeneratorMetadata meta;
    meta.name = "g";
    meta.module = "m";
    meta.supports_unique = true;
    meta.supports_data_linkage = true;
    meta.config_template = Json::object();
    meta.config_template["a_bool"] = true;
    meta.config_template["a_array"] = Json::array({1, 2});
    meta.config_template["a_str"] = "x";
    meta.config_params.push_back(data_generator::config::ConfigParam{"a_bool", "boolean", "desc", false, {}});
    meta.config_params.push_back(data_generator::config::ConfigParam{"a_array", "", "desc", false, {}});
    meta.config_params.push_back(data_generator::config::ConfigParam{"a_str", "string", "desc", false, {"x", "y"}});

    const auto lines = build_describe_text_lines(meta);
    EXPECT_FALSE(lines.empty());
}

TEST(CliSharedTest, RequiredFlagsOnlyUseTranslationCanBeFalse) {
    for (const auto& meta : data_generator::config::get_generator_catalog()) {
        for (const auto& param : meta.config_params) {
            if (param.name == "use_translation") {
                EXPECT_FALSE(param.required);
            } else {
                EXPECT_TRUE(param.required) << "generator=" << meta.name << " param=" << param.name;
            }
        }
    }
}

TEST(CliSharedTest, BuildJsonSchemaUsesCatalogMetadata) {
    const Json schema = build_json_schema();

    ASSERT_TRUE(schema.contains("$schema"));
    EXPECT_EQ(schema.at("$schema").get<std::string>(), "https://json-schema.org/draft/2020-12/schema");

    ASSERT_TRUE(schema.contains("required"));
    EXPECT_NE(std::find(schema.at("required").begin(), schema.at("required").end(), "fields"), schema.at("required").end());

    ASSERT_TRUE(schema.contains("properties"));
    const Json& properties = schema.at("properties");
    ASSERT_TRUE(properties.contains("destination"));
    EXPECT_EQ(properties.at("destination").at("type").get<std::string>(), "string");
    ASSERT_TRUE(properties.contains("file_format"));
    EXPECT_EQ(properties.at("file_format").at("type").get<std::string>(), "string");
    ASSERT_TRUE(properties.contains("fields"));
    const Json& fields_schema = properties.at("fields");
    EXPECT_EQ(fields_schema.at("type").get<std::string>(), "array");
    ASSERT_TRUE(fields_schema.contains("items"));
    const Json& item_schema = fields_schema.at("items");
    ASSERT_TRUE(item_schema.contains("properties"));
    ASSERT_TRUE(item_schema.at("properties").contains("generator"));
    const Json& generator_schema = item_schema.at("properties").at("generator");
    EXPECT_EQ(generator_schema.at("type").get<std::string>(), "string");
    const auto& catalog = data_generator::config::get_generator_catalog();
    const Json& generator_enum = generator_schema.at("enum");
    EXPECT_EQ(generator_enum.size(), catalog.size());
    for (const auto& meta : catalog) {
        EXPECT_NE(std::find(generator_enum.begin(), generator_enum.end(), meta.name), generator_enum.end());
    }

    ASSERT_TRUE(item_schema.contains("allOf"));
    const Json& all_of = item_schema.at("allOf");
    ASSERT_TRUE(all_of.is_array());
    EXPECT_EQ(all_of.size(), catalog.size());

    const Json* company_constraint = find_generator_constraint(all_of, "company_name");
    ASSERT_NE(company_constraint, nullptr);
    ASSERT_TRUE(company_constraint->contains("then"));
    const Json& company_then   = company_constraint->at("then");
    const Json& company_config = company_then.at("properties").at("config");
    const Json& company_props  = company_config.at("properties");
    EXPECT_EQ(company_props.at("languages").at("type").get<std::string>(), "array");
    EXPECT_EQ(company_props.at("languages").at("items").at("type").get<std::string>(), "string");
    EXPECT_TRUE(company_props.at("languages").at("items").contains("enum"));
    EXPECT_EQ(company_props.at("use_translation").at("type").get<std::string>(), "boolean");
    ASSERT_TRUE(company_config.contains("required"));
    EXPECT_NE(
        std::find(company_config.at("required").begin(), company_config.at("required").end(), "languages"),
        company_config.at("required").end()
    );
    ASSERT_TRUE(company_then.contains("allOf"));
    EXPECT_TRUE(contains_not_required_key(company_then.at("allOf"), "unique"));
    EXPECT_FALSE(contains_not_required_key(company_then.at("allOf"), "data_linkage"));

    const Json* ip_address_constraint = find_generator_constraint(all_of, "ip_address");
    ASSERT_NE(ip_address_constraint, nullptr);
    ASSERT_TRUE(ip_address_constraint->at("then").contains("allOf"));
    EXPECT_FALSE(contains_not_required_key(ip_address_constraint->at("then").at("allOf"), "unique"));
    EXPECT_TRUE(contains_not_required_key(ip_address_constraint->at("then").at("allOf"), "data_linkage"));

    const Json* decimal_constraint = find_generator_constraint(all_of, "decimal");
    ASSERT_NE(decimal_constraint, nullptr);
    const Json& decimal_props = decimal_constraint->at("then").at("properties").at("config").at("properties");
    EXPECT_EQ(decimal_props.at("start").at("type").get<std::string>(), "number");
}

}  // namespace
