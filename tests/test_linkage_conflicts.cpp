/// @file test_linkage_conflicts.cpp

#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include <sstream>
#include <vector>

#include "core/configuration.h"
#include "core/executor.h"

using namespace data_generator::core;

namespace {

GenerationConfig cfg_from_json(const char* text) {
    GenerationConfig cfg;
    std::vector<ValidationIssue> issues;
    const auto root = nlohmann::json::parse(text);
    const bool ok = parse_generation_config(root, ParseMode::RequireOutputSettings, &cfg, &issues);
    EXPECT_TRUE(ok);
    return cfg;
}

TEST(LinkageConflictTest, PersonConflictBranches) {
    const auto cfg = cfg_from_json(R"json(
{
  "rows": 2,
  "output_format": "json",
  "fields": [
    {"name":"f1","generator":"first_name","config":{"languages":["English"],"genders":["M"],"use_translation":false},"data_linkage":"person:x"},
    {"name":"f2","generator":"last_name","config":{"languages":["Japanese"],"use_translation":false},"data_linkage":"person:x"}
  ]
}
)json");
    std::ostringstream out;
    EXPECT_THROW(
        (void)generate_to_stream(cfg, ExecutionOptions{.requested_threads = 1, .seed = std::nullopt}, out),
        std::runtime_error
    );

    const auto cfg_gender = cfg_from_json(R"json(
{
  "rows": 2,
  "output_format": "json",
  "fields": [
    {"name":"f1","generator":"first_name","config":{"languages":["English"],"genders":["M"],"use_translation":false},"data_linkage":"person:g"},
    {"name":"f2","generator":"first_name","config":{"languages":["English"],"genders":["F"],"use_translation":false},"data_linkage":"person:g"}
  ]
}
)json");
    std::ostringstream out_gender;
    EXPECT_THROW(
        (void)generate_to_stream(cfg_gender, ExecutionOptions{.requested_threads = 1, .seed = std::nullopt}, out_gender),
        std::runtime_error
    );
}

TEST(LinkageConflictTest, PersonConflictRegionsDomainsAndUnique) {
    const auto cfg_regions = cfg_from_json(R"json(
{
  "rows": 2,
  "output_format": "json",
  "fields": [
    {"name":"p1","generator":"phone_number","config":{"regions":["United States"]},"data_linkage":"person:r"},
    {"name":"p2","generator":"phone_number","config":{"regions":["China"]},"data_linkage":"person:r"}
  ]
}
)json");
    std::ostringstream out1;
    EXPECT_THROW(
        (void)generate_to_stream(cfg_regions, ExecutionOptions{.requested_threads = 1, .seed = std::nullopt}, out1),
        std::runtime_error
    );

    const auto cfg_domains = cfg_from_json(R"json(
{
  "rows": 2,
  "output_format": "json",
  "fields": [
    {"name":"e1","generator":"email","unique":true,"config":{"languages":["English"],"domains":["a.com"]},"data_linkage":"person:d"},
    {"name":"e2","generator":"email","unique":true,"config":{"languages":["English"],"domains":["b.com"]},"data_linkage":"person:d"}
  ]
}
)json");
    std::ostringstream out2;
    EXPECT_THROW(
        (void)generate_to_stream(cfg_domains, ExecutionOptions{.requested_threads = 1, .seed = std::nullopt}, out2),
        std::runtime_error
    );

    const auto cfg_unique = cfg_from_json(R"json(
{
  "rows": 2,
  "output_format": "json",
  "fields": [
    {"name":"e1","generator":"email","unique":true,"config":{"languages":["English"],"domains":["a.com"]},"data_linkage":"person:u"},
    {"name":"e2","generator":"email","unique":false,"config":{"languages":["English"],"domains":["a.com"]},"data_linkage":"person:u"}
  ]
}
)json");
    std::ostringstream out3;
    EXPECT_THROW(
        (void)generate_to_stream(cfg_unique, ExecutionOptions{.requested_threads = 1, .seed = std::nullopt}, out3),
        std::runtime_error
    );
}

TEST(LinkageConflictTest, PaymentConflictBranches) {
    const auto cfg = cfg_from_json(R"json(
{
  "rows": 2,
  "output_format": "json",
  "fields": [
    {"name":"c1","generator":"card_type","config":{"languages":["English"],"card_types":["Visa"]},"data_linkage":"payment:x"},
    {"name":"c2","generator":"card_type","config":{"languages":["Japanese"],"card_types":["Visa"]},"data_linkage":"payment:x"}
  ]
}
)json");
    std::ostringstream out;
    EXPECT_THROW(
        (void)generate_to_stream(cfg, ExecutionOptions{.requested_threads = 1, .seed = std::nullopt}, out),
        std::runtime_error
    );

    const auto cfg_card_types = cfg_from_json(R"json(
{
  "rows": 2,
  "output_format": "json",
  "fields": [
    {"name":"n1","generator":"card_number","config":{"card_types":["Visa"]},"data_linkage":"payment:c"},
    {"name":"n2","generator":"card_number","config":{"card_types":["MasterCard"]},"data_linkage":"payment:c"}
  ]
}
)json");
    std::ostringstream out_card_types;
    EXPECT_THROW(
        (void)generate_to_stream(
            cfg_card_types,
            ExecutionOptions{.requested_threads = 1, .seed = std::nullopt},
            out_card_types
        ),
        std::runtime_error
    );
}

TEST(LinkageConflictTest, PaymentConflictCardDateAndUnique) {
    const auto cfg_start = cfg_from_json(R"json(
{
  "rows": 2,
  "output_format": "json",
  "fields": [
    {"name":"d1","generator":"card_date","config":{"start_month":"01/20","end_month":"12/30"},"data_linkage":"payment:s"},
    {"name":"d2","generator":"card_date","config":{"start_month":"02/20","end_month":"12/30"},"data_linkage":"payment:s"}
  ]
}
)json");
    std::ostringstream out1;
    EXPECT_THROW(
        (void)generate_to_stream(cfg_start, ExecutionOptions{.requested_threads = 1, .seed = std::nullopt}, out1),
        std::runtime_error
    );

    const auto cfg_end = cfg_from_json(R"json(
{
  "rows": 2,
  "output_format": "json",
  "fields": [
    {"name":"d1","generator":"card_date","config":{"start_month":"01/20","end_month":"11/30"},"data_linkage":"payment:e"},
    {"name":"d2","generator":"card_date","config":{"start_month":"01/20","end_month":"12/30"},"data_linkage":"payment:e"}
  ]
}
)json");
    std::ostringstream out2;
    EXPECT_THROW(
        (void)generate_to_stream(cfg_end, ExecutionOptions{.requested_threads = 1, .seed = std::nullopt}, out2),
        std::runtime_error
    );

    const auto cfg_unique = cfg_from_json(R"json(
{
  "rows": 2,
  "output_format": "json",
  "fields": [
    {"name":"n1","generator":"card_number","unique":true,"config":{"card_types":["Visa"]},"data_linkage":"payment:u"},
    {"name":"n2","generator":"card_number","unique":false,"config":{"card_types":["Visa"]},"data_linkage":"payment:u"}
  ]
}
)json");
    std::ostringstream out3;
    EXPECT_THROW(
        (void)generate_to_stream(cfg_unique, ExecutionOptions{.requested_threads = 1, .seed = std::nullopt}, out3),
        std::runtime_error
    );
}

TEST(LinkageConflictTest, LocationAndComputerConflictBranches) {
    const auto cfg1 = cfg_from_json(R"json(
{
  "rows": 2,
  "output_format": "json",
  "fields": [
    {"name":"l1","generator":"address_line1","config":{"regions":["United States"],"use_translation":false},"data_linkage":"location:x"},
    {"name":"l2","generator":"city","config":{"regions":["China"],"use_translation":false},"data_linkage":"location:x"}
  ]
}
)json");
    std::ostringstream out1;
    EXPECT_THROW(
        (void)generate_to_stream(cfg1, ExecutionOptions{.requested_threads = 1, .seed = std::nullopt}, out1),
        std::runtime_error
    );

    const auto cfg2 = cfg_from_json(R"json(
{
  "rows": 2,
  "output_format": "json",
  "fields": [
    {"name":"f1","generator":"file_path","config":{"operating_systems":["Windows"],"extensions":["txt"]},"data_linkage":"file:x"},
    {"name":"f2","generator":"file_name","config":{"extensions":["csv"]},"data_linkage":"file:x"}
  ]
}
)json");
    std::ostringstream out2;
    EXPECT_THROW(
        (void)generate_to_stream(cfg2, ExecutionOptions{.requested_threads = 1, .seed = std::nullopt}, out2),
        std::runtime_error
    );

    const auto cfg3 = cfg_from_json(R"json(
{
  "rows": 2,
  "output_format": "json",
  "fields": [
    {"name":"f1","generator":"file_path","config":{"operating_systems":["Windows"],"extensions":["txt"]},"data_linkage":"file:os"},
    {"name":"f2","generator":"file_path","config":{"operating_systems":["Linux"],"extensions":["txt"]},"data_linkage":"file:os"}
  ]
}
)json");
    std::ostringstream out3;
    EXPECT_THROW(
        (void)generate_to_stream(cfg3, ExecutionOptions{.requested_threads = 1, .seed = std::nullopt}, out3),
        std::runtime_error
    );
}

TEST(LinkageConflictTest, ComputerAndUtilityErrorBranches) {
    GenerationConfig cfg;
    std::vector<ValidationIssue> issues;

    const auto root1 = nlohmann::json::parse(R"json(
{
  "rows": 1,
  "output_format": "json",
  "fields": [
    {"name":"u","generator":"url","config":{"subdomains":["www"]}}
  ]
}
)json");
    EXPECT_TRUE(parse_generation_config(root1, ParseMode::RequireOutputSettings, &cfg, &issues));

    const auto root2 = nlohmann::json::parse(R"json(
{
  "rows": 1,
  "output_format": "json",
  "fields": [
    {"name":"h","generator":"hostname","config":{"subdomains":["www"]}}
  ]
}
)json");
    EXPECT_TRUE(parse_generation_config(root2, ParseMode::RequireOutputSettings, &cfg, &issues));

    const auto cfg_invalid_url = cfg_from_json(R"json(
{
  "rows": 1,
  "output_format": "json",
  "fields": [
    {"name":"u","generator":"url","config":{"subdomains":["www"],"tlds":[]}}
  ]
}
)json");
    std::ostringstream out_invalid_url;
    EXPECT_THROW(
        (void)generate_to_stream(
            cfg_invalid_url,
            ExecutionOptions{.requested_threads = 1, .seed = std::nullopt},
            out_invalid_url
        ),
        std::invalid_argument
    );

    const auto cfg_invalid_host = cfg_from_json(R"json(
{
  "rows": 1,
  "output_format": "json",
  "fields": [
    {"name":"h","generator":"hostname","config":{"subdomains":["www"],"tlds":[]}}
  ]
}
)json");
    std::ostringstream out_invalid_host;
    EXPECT_THROW(
        (void)generate_to_stream(
            cfg_invalid_host,
            ExecutionOptions{.requested_threads = 1, .seed = std::nullopt},
            out_invalid_host
        ),
        std::invalid_argument
    );

    const auto cfg3 = cfg_from_json(R"json(
{
  "rows": 1,
  "output_format": "json",
  "fields": [
    {"name":"d","generator":"file_directory","config":{"operating_systems":["Windows"]},"data_linkage":"file:missing"}
  ]
}
)json");
    std::ostringstream out3;
    EXPECT_THROW(
        (void)generate_to_stream(cfg3, ExecutionOptions{.requested_threads = 1, .seed = std::nullopt}, out3),
        std::runtime_error
    );
}

TEST(LinkageConflictTest, FileExtensionLinkagePath) {
    const auto cfg = cfg_from_json(R"json(
{
  "rows": 3,
  "output_format": "json",
  "fields": [
    {"name":"p","generator":"file_path","config":{"operating_systems":["Windows"],"extensions":["txt"]},"data_linkage":"file:ext"},
    {"name":"e","generator":"file_extension","config":{"extensions":["txt"]},"data_linkage":"file:ext"}
  ]
}
)json");
    std::ostringstream out;
    EXPECT_NO_THROW(
        (void)generate_to_stream(cfg, ExecutionOptions{.requested_threads = 1, .seed = std::nullopt}, out)
    );
    EXPECT_NE(out.str().find("\"e\""), std::string::npos);
}

}  // namespace
