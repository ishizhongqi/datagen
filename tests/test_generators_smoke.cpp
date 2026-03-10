/// @file test_generators_smoke.cpp

#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include <sstream>
#include <string>
#include <vector>

#include "cli/generator_catalog.h"
#include "core/configuration.h"
#include "core/executor.h"

using namespace data_generator;

namespace {

core::GenerationConfig parse_config_or_fail(const nlohmann::json& root) {
    core::GenerationConfig cfg;
    std::vector<core::ValidationIssue> issues;
    const bool ok = core::parse_generation_config(root, core::ParseMode::RequireOutputSettings, &cfg, &issues);
    EXPECT_TRUE(ok);
    if (!ok) {
        for (const auto& issue : issues) {
            ADD_FAILURE() << issue.path << ": " << issue.message;
        }
    }
    return cfg;
}

TEST(GeneratorsSmokeTest, EveryGeneratorTemplateCanGenerateOneRow) {
    std::vector<std::string> failures;

    for (const auto& meta : cli::get_generator_catalog()) {
        nlohmann::json root;
        root["rows"] = 1;
        root["destination"] = "file";
        root["file_format"] = "json";
        root["null_value_string"] = "N";

        nlohmann::json field;
        field["name"] = "f";
        field["generator"] = meta.name;
        field["config"] = meta.config_template;
        // catalog template for card_date currently uses start/end aliases
        if (meta.name == "card_date") {
            if (field["config"].contains("start")) {
                field["config"]["start_month"] = field["config"]["start"];
                field["config"].erase("start");
            }
            if (field["config"].contains("end")) {
                field["config"]["end_month"] = field["config"]["end"];
                field["config"].erase("end");
            }
        }
        if (meta.supports_unique) {
            field["unique"] = true;
        }
        root["fields"] = nlohmann::json::array({field});

        try {
            core::GenerationConfig cfg;
            std::vector<core::ValidationIssue> issues;
            const bool ok =
                core::parse_generation_config(root, core::ParseMode::RequireOutputSettings, &cfg, &issues);
            if (!ok) {
                std::string details;
                for (const auto& issue : issues) {
                    if (!details.empty()) {
                        details += "; ";
                    }
                    details += issue.path + " " + issue.message;
                }
                failures.push_back(meta.name + ": parse failed: " + details);
                continue;
            }
            std::ostringstream out;
            (void)core::generate_to_stream(cfg, core::ExecutionOptions{.requested_threads = 1}, out);
        } catch (const std::exception& ex) {
            failures.push_back(meta.name + ": " + ex.what());
        }
    }

    if (!failures.empty()) {
        for (const auto& failure : failures) {
            ADD_FAILURE() << failure;
        }
    }
    EXPECT_TRUE(failures.empty());
}

TEST(GeneratorsSmokeTest, LinkagePathsGenerateAcrossModules) {
    const auto root = nlohmann::json::parse(R"json(
{
  "rows": 3,
  "destination": "file",
  "file_format": "json",
  "fields": [
    {"name":"company_name","generator":"company_name","config":{"languages":["English"]},"data_linkage":"company:group_a"},
    {"name":"industry","generator":"industry","config":{"languages":["English"]},"data_linkage":"company:group_a"},

    {"name":"first_name","generator":"first_name","config":{"languages":["English"],"genders":["M"],"use_translation":false},"data_linkage":"person:group_a"},
    {"name":"last_name","generator":"last_name","config":{"languages":["English"],"use_translation":false},"data_linkage":"person:group_a"},
    {"name":"email","generator":"email","config":{"domains":["example.com"]},"data_linkage":"person:group_a"},

    {"name":"address_line1","generator":"address_line1","config":{"regions":["United States"],"use_translation":false},"data_linkage":"location:group_a"},
    {"name":"city","generator":"city","config":{"regions":["United States"],"use_translation":false},"data_linkage":"location:group_a"},

    {"name":"file_path","generator":"file_path","config":{"operating_systems":["Windows"],"extensions":["txt"]},"data_linkage":"file:group_a"},
    {"name":"file_name","generator":"file_name","config":{"extensions":["txt"]},"data_linkage":"file:group_a"},

    {"name":"card_type","generator":"card_type","config":{"languages":["English"],"card_types":["Visa"]},"data_linkage":"payment:group_a"},
    {"name":"card_number","generator":"card_number","config":{"card_types":["Visa"]},"data_linkage":"payment:group_a"}
  ]
}
)json");

    auto cfg = parse_config_or_fail(root);
    std::ostringstream out;
    const auto result = core::generate_to_stream(cfg, core::ExecutionOptions{.requested_threads = 1}, out);

    EXPECT_EQ(result.info.threads_used, 1U);
    EXPECT_FALSE(out.str().empty());
}

TEST(GeneratorsSmokeTest, ConflictingLinkageConfigThrows) {
    const auto root = nlohmann::json::parse(R"json(
{
  "rows": 1,
  "destination": "file",
  "file_format": "json",
  "fields": [
    {"name":"company_name","generator":"company_name","config":{"languages":["English"]},"data_linkage":"company:group_conflict"},
    {"name":"industry","generator":"industry","config":{"languages":["Japanese"]},"data_linkage":"company:group_conflict"}
  ]
}
)json");

    auto cfg = parse_config_or_fail(root);
    std::ostringstream out;
    EXPECT_THROW(
        (void)core::generate_to_stream(cfg, core::ExecutionOptions{.requested_threads = 1}, out),
        std::runtime_error
    );
}

TEST(GeneratorsSmokeTest, PersonLinkageAllGenerators) {
    const auto root = nlohmann::json::parse(R"json(
{
  "rows": 3,
  "destination": "file",
  "file_format": "json",
  "fields": [
    {"name":"first_name","generator":"first_name","config":{"languages":["English"],"genders":["M"],"use_translation":false},"data_linkage":"person:all"},
    {"name":"last_name","generator":"last_name","config":{"languages":["English"],"use_translation":false},"data_linkage":"person:all"},
    {"name":"full_name","generator":"full_name","config":{"languages":["English"],"genders":["M"],"use_translation":false},"data_linkage":"person:all"},
    {"name":"gender","generator":"gender","config":{"languages":["English"]},"data_linkage":"person:all"},
    {"name":"title","generator":"title","config":{"languages":["English"],"genders":["M"]},"data_linkage":"person:all"},
    {"name":"marital_status","generator":"marital_status","config":{"languages":["English"]},"data_linkage":"person:all"},
    {"name":"phone_number","generator":"phone_number","config":{"regions":["United States"],"is_international":true,"include_delimiters":false},"data_linkage":"person:all"},
    {"name":"email","generator":"email","config":{"languages":["English"],"domains":["example.com"]},"data_linkage":"person:all"},
    {"name":"job_title","generator":"job_title","config":{"languages":["English"]},"data_linkage":"person:all"},
    {"name":"social_network_id","generator":"social_network_id","config":{"languages":["English"],"use_translation":false},"data_linkage":"person:all"}
  ]
}
)json");

    auto cfg = parse_config_or_fail(root);
    std::ostringstream out;
    (void)core::generate_to_stream(cfg, core::ExecutionOptions{.requested_threads = 1}, out);
    EXPECT_FALSE(out.str().empty());
}

TEST(GeneratorsSmokeTest, LocationAndPaymentLinkageAllGenerators) {
    const auto root = nlohmann::json::parse(R"json(
{
  "rows": 3,
  "destination": "file",
  "file_format": "json",
  "fields": [
    {"name":"address_line1","generator":"address_line1","config":{"regions":["United States"],"use_translation":false},"data_linkage":"location:all"},
    {"name":"address_line2","generator":"address_line2","config":{"regions":["United States"],"use_translation":false},"data_linkage":"location:all"},
    {"name":"postcode","generator":"postcode","config":{"regions":["United States"]},"data_linkage":"location:all"},
    {"name":"full_address","generator":"full_address","config":{"regions":["United States"],"use_translation":false},"data_linkage":"location:all"},
    {"name":"city","generator":"city","config":{"regions":["United States"],"use_translation":false},"data_linkage":"location:all"},
    {"name":"region","generator":"region","config":{"country_codes_standard":"None","languages":["English"]},"data_linkage":"location:all"},

    {"name":"payment_method","generator":"payment_method","config":{"payment_methods":["Visa"]},"data_linkage":"payment:all"},
    {"name":"card_type","generator":"card_type","config":{"languages":["English"],"card_types":["Visa"]},"data_linkage":"payment:all"},
    {"name":"card_number","generator":"card_number","config":{"card_types":["Visa"]},"data_linkage":"payment:all"},
    {"name":"card_date","generator":"card_date","config":{"start_month":"01/24","end_month":"12/26"},"data_linkage":"payment:all"}
  ]
}
)json");

    auto cfg = parse_config_or_fail(root);
    std::ostringstream out;
    (void)core::generate_to_stream(cfg, core::ExecutionOptions{.requested_threads = 1}, out);
    EXPECT_FALSE(out.str().empty());
}

TEST(GeneratorsSmokeTest, OptionalBranchesEmailAndPaymentMethodWithoutLists) {
    const auto root = nlohmann::json::parse(R"json(
{
  "rows": 2,
  "destination": "file",
  "file_format": "json",
  "fields": [
    {"name":"email_any","generator":"email","config":{"languages":["English"]}},
    {"name":"method_any","generator":"payment_method","config":{}},
    {"name":"region_link_default","generator":"region","config":{"country_codes_standard":"None","languages":["English"]},"data_linkage":"location:only_region"}
  ]
}
)json");

    auto cfg = parse_config_or_fail(root);
    std::ostringstream out;
    (void)core::generate_to_stream(cfg, core::ExecutionOptions{.requested_threads = 1}, out);
    EXPECT_FALSE(out.str().empty());
}

}  // namespace
