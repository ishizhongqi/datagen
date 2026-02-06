// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file generator_catalog.cpp

#include "cli/generator_catalog.h"

#include <algorithm>
#include <utility>

namespace data_generator::cli {

namespace {

GeneratorMetadata make_metadata(
    std::string              name,
    std::vector<ConfigParam> params,
    bool                     supports_unique,
    bool                     supports_data_linkage,
    std::string              linkage_module,
    Json                     config_template
) {
    GeneratorMetadata meta;
    meta.name                  = std::move(name);
    meta.config_params         = std::move(params);
    meta.supports_unique       = supports_unique;
    meta.supports_data_linkage = supports_data_linkage;
    meta.linkage_module        = std::move(linkage_module);
    meta.config_template       = std::move(config_template);
    return meta;
}

const std::vector<GeneratorMetadata>& build_catalog() {
    static const std::vector catalog = {
        make_metadata(
            "company_name",
            {
                {"languages", "Language names (string or array)."},
                {"use_translation", "Whether to use translated output."},
            },
            false,
            true,
            "company",
            Json{{"languages", Json::array({"English"})}, {"use_translation", false}}
        ),
        make_metadata(
            "department",
            {
                {"languages", "Language names (string or array)."},
            },
            false,
            false,
            "",
            Json{{"languages", Json::array({"English"})}}
        ),
        make_metadata(
            "industry",
            {
                {"languages", "Language names (string or array)."},
            },
            false,
            true,
            "company",
            Json{{"languages", Json::array({"English"})}}
        ),
        make_metadata(
            "ip_address",
            {
                {"ip_address_type", "IPv4 or IPv6."},
            },
            true,
            false,
            "",
            Json{{"ip_address_type", "IPv4"}}
        ),
        make_metadata("mac_address", {}, true, false, "", Json::object()),
        make_metadata(
            "file_path",
            {
                {"operating_systems", "Operating systems (string or array)."},
                {"extensions", "File extensions (string or array)."},
            },
            false,
            true,
            "file",
            Json{
                {"operating_systems", Json::array({"Windows"})},
                {"extensions", Json::array({"txt", "csv"})},
            }
        ),
        make_metadata(
            "file_directory",
            {
                {"operating_systems", "Operating systems (string or array)."},
            },
            false,
            true,
            "file",
            Json{{"operating_systems", Json::array({"Windows"})}}
        ),
        make_metadata(
            "file_name",
            {
                {"extensions", "File extensions (string or array)."},
            },
            false,
            true,
            "file",
            Json{{"extensions", Json::array({"txt", "csv"})}}
        ),
        make_metadata(
            "file_extension",
            {
                {"extensions", "File extensions (string or array)."},
            },
            false,
            true,
            "file",
            Json{{"extensions", Json::array({"txt", "csv"})}}
        ),
        make_metadata(
            "url",
            {
                {"subdomains", "Subdomains (string or array)."},
                {"tlds", "Top-level domains (string or array)."},
            },
            true,
            false,
            "",
            Json{{"subdomains", Json::array({"www"})}, {"tlds", Json::array({"com"})}}
        ),
        make_metadata(
            "hostname",
            {
                {"subdomains", "Subdomains (string or array)."},
                {"tlds", "Top-level domains (string or array)."},
            },
            true,
            false,
            "",
            Json{{"subdomains", Json::array({"www"})}, {"tlds", Json::array({"com"})}}
        ),
        make_metadata(
            "date",
            {
                {"start_date", "Start date in YYYY-MM-DD."},
                {"end_date", "End date in YYYY-MM-DD."},
                {"days_of_week", "Days to include (string or array)."},
            },
            false,
            false,
            "",
            Json{
                {"start_date", "2023-01-01"},
                {"end_date", "2023-12-31"},
                {"days_of_week", Json::array({"Monday", "Tuesday", "Wednesday", "Thursday", "Friday"})},
            }
        ),
        make_metadata(
            "time",
            {
                {"start_time", "Start time in HH:MM:SS."},
                {"end_time", "End time in HH:MM:SS."},
            },
            false,
            false,
            "",
            Json{{"start_time", "08:00:00"}, {"end_time", "17:00:00"}}
        ),
        make_metadata(
            "datetime",
            {
                {"start_date", "Start date in YYYY-MM-DD."},
                {"end_date", "End date in YYYY-MM-DD."},
                {"start_time", "Start time in HH:MM:SS."},
                {"end_time", "End time in HH:MM:SS."},
                {"days_of_week", "Days to include (string or array)."},
            },
            false,
            false,
            "",
            Json{
                {"start_date", "2023-01-01"},
                {"end_date", "2023-12-31"},
                {"start_time", "08:00:00"},
                {"end_time", "17:00:00"},
                {"days_of_week", Json::array({"Monday", "Tuesday", "Wednesday", "Thursday", "Friday"})},
            }
        ),
        make_metadata(
            "address_line1",
            {
                {"regions", "Regions (string or array)."},
                {"use_translation", "Whether to use translated output."},
            },
            false,
            true,
            "location",
            Json{{"regions", Json::array({"United States"})}, {"use_translation", false}}
        ),
        make_metadata(
            "address_line2",
            {
                {"regions", "Regions (string or array)."},
                {"use_translation", "Whether to use translated output."},
            },
            false,
            true,
            "location",
            Json{{"regions", Json::array({"United States"})}, {"use_translation", false}}
        ),
        make_metadata(
            "postcode",
            {
                {"regions", "Regions (string or array)."},
            },
            false,
            true,
            "location",
            Json{{"regions", Json::array({"United States"})}}
        ),
        make_metadata(
            "full_address",
            {
                {"regions", "Regions (string or array)."},
                {"use_translation", "Whether to use translated output."},
            },
            false,
            true,
            "location",
            Json{{"regions", Json::array({"United States"})}, {"use_translation", false}}
        ),
        make_metadata(
            "city",
            {
                {"regions", "Regions (string or array)."},
                {"use_translation", "Whether to use translated output."},
            },
            false,
            true,
            "location",
            Json{{"regions", Json::array({"United States"})}, {"use_translation", false}}
        ),
        make_metadata(
            "region",
            {
                {"country_codes_standard", "Country code standard string."},
                {"languages", "Language names (string or array)."},
            },
            false,
            false,
            "",
            Json{{"country_codes_standard", "None"}, {"languages", Json::array({"English"})}}
        ),
        make_metadata(
            "integer",
            {
                {"start", "Start value."},
                {"end", "End value."},
            },
            false,
            false,
            "",
            Json{{"start", 1}, {"end", 100}}
        ),
        make_metadata(
            "unsigned_integer",
            {
                {"start", "Start value."},
                {"end", "End value."},
            },
            false,
            false,
            "",
            Json{{"start", 1}, {"end", 100}}
        ),
        make_metadata(
            "decimal",
            {
                {"start", "Start value."},
                {"end", "End value."},
                {"decimal_places", "Number of decimal places."},
            },
            false,
            false,
            "",
            Json{{"start", 1.0}, {"end", 100.0}, {"decimal_places", 2}}
        ),
        make_metadata(
            "decimal_string",
            {
                {"start", "Start value."},
                {"end", "End value."},
                {"decimal_places", "Number of decimal places."},
            },
            false,
            false,
            "",
            Json{{"start", 1.0}, {"end", 100.0}, {"decimal_places", 2}}
        ),
        make_metadata(
            "payment_method",
            {
                {"payment_methods", "Payment methods (string or array)."},
            },
            false,
            true,
            "card",
            Json{{"payment_methods", Json::array({"Credit Card", "PayPal"})}}
        ),
        make_metadata(
            "card_type",
            {
                {"languages", "Language names (string or array)."},
                {"card_types", "Card types (string or array)."},
                {"card_type", "Single card type (string)."},
            },
            false,
            true,
            "card",
            Json{{"languages", Json::array({"English"})}, {"card_types", Json::array({"Visa", "MasterCard"})}}
        ),
        make_metadata(
            "card_number",
            {
                {"card_types", "Card types (string or array)."},
                {"card_type", "Single card type (string)."},
            },
            true,
            true,
            "card",
            Json{{"card_types", Json::array({"Visa", "MasterCard"})}}
        ),
        make_metadata(
            "card_date",
            {
                {"start", "Start date in MM/YY."},
                {"end", "End date in MM/YY."},
            },
            false,
            true,
            "card",
            Json{{"start", "01/23"}, {"end", "12/30"}}
        ),
        make_metadata(
            "first_name",
            {
                {"languages", "Language names (string or array)."},
                {"genders", "Genders (string or array)."},
                {"use_translation", "Whether to use translated output."},
            },
            false,
            true,
            "person",
            Json{
                {"languages", Json::array({"English"})},
                {"genders", Json::array({"M", "F"})},
                {"use_translation", false},
            }
        ),
        make_metadata(
            "last_name",
            {
                {"languages", "Language names (string or array)."},
                {"use_translation", "Whether to use translated output."},
            },
            false,
            true,
            "person",
            Json{{"languages", Json::array({"English"})}, {"use_translation", false}}
        ),
        make_metadata(
            "full_name",
            {
                {"languages", "Language names (string or array)."},
                {"genders", "Genders (string or array)."},
                {"use_translation", "Whether to use translated output."},
            },
            false,
            true,
            "person",
            Json{
                {"languages", Json::array({"English"})},
                {"genders", Json::array({"M", "F"})},
                {"use_translation", false},
            }
        ),
        make_metadata(
            "gender",
            {
                {"languages", "Language names (string or array)."},
            },
            false,
            true,
            "person",
            Json{{"languages", Json::array({"English"})}}
        ),
        make_metadata(
            "title",
            {
                {"languages", "Language names (string or array)."},
                {"genders", "Genders (string or array)."},
            },
            false,
            true,
            "person",
            Json{{"languages", Json::array({"English"})}, {"genders", Json::array({"M", "F"})}}
        ),
        make_metadata(
            "marital_status",
            {
                {"languages", "Language names (string or array)."},
            },
            false,
            true,
            "person",
            Json{{"languages", Json::array({"English"})}}
        ),
        make_metadata(
            "phone_number",
            {
                {"regions", "Regions (string or array)."},
                {"is_international", "Whether to generate international numbers."},
                {"include_delimiters", "Whether to include delimiters."},
            },
            true,
            true,
            "person",
            Json{{"regions", Json::array({"United States"})}, {"is_international", false}, {"include_delimiters", true}}
        ),
        make_metadata(
            "email",
            {
                {"languages", "Language names (string or array)."},
                {"domains", "Email domains (string or array)."},
            },
            true,
            true,
            "person",
            Json{{"languages", Json::array({"English"})}, {"domains", Json::array({"example.com"})}}
        ),
        make_metadata(
            "job_title",
            {
                {"languages", "Language names (string or array)."},
            },
            false,
            true,
            "person",
            Json{{"languages", Json::array({"English"})}}
        ),
        make_metadata(
            "social_network_id",
            {
                {"languages", "Language names (string or array)."},
                {"use_translation", "Whether to use translated output."},
            },
            true,
            true,
            "person",
            Json{{"languages", Json::array({"English"})}, {"use_translation", false}}
        ),
        make_metadata(
            "product_name",
            {
                {"languages", "Language names (string or array)."},
                {"keywords", "Keyword list (string or array)."},
            },
            false,
            false,
            "",
            Json{{"languages", Json::array({"English"})}, {"keywords", Json::array({"Cherry", "Orange"})}}
        ),
        make_metadata(
            "product_category",
            {
                {"languages", "Language names (string or array)."},
            },
            false,
            false,
            "",
            Json{{"languages", Json::array({"English"})}}
        ),
        make_metadata(
            "color",
            {
                {"languages", "Language names (string or array)."},
            },
            false,
            false,
            "",
            Json{{"languages", Json::array({"English"})}}
        ),
        make_metadata("size", {}, false, false, "", Json::object()),
        make_metadata(
            "barcode",
            {
                {"barcode_types", "Barcode types (string or array)."},
                {"barcode_type", "Single barcode type (string)."},
            },
            true,
            false,
            "",
            Json{{"barcode_types", Json::array({"EAN13"})}}
        ),
        make_metadata(
            "enum_item",
            {
                {"enums", "Enum values (string or array)."},
            },
            false,
            false,
            "",
            Json{{"enums", Json::array({"First", "Second", "Third"})}}
        ),
        make_metadata(
            "text",
            {
                {"number_of_chars_start", "Minimum number of characters."},
                {"number_of_chars_end", "Maximum number of characters."},
            },
            false,
            false,
            "",
            Json{{"number_of_chars_start", 100}, {"number_of_chars_end", 1000}}
        ),
        make_metadata(
            "uuid",
            {
                {"include_hyphens", "Whether to include hyphens."},
            },
            false,
            false,
            "",
            Json{{"include_hyphens", true}}
        ),
        make_metadata(
            "sequence",
            {
                {"start", "Start value."},
                {"end", "End value."},
                {"step", "Step increment (non-zero)."},
                {"circle", "Whether to wrap when reaching end."},
            },
            false,
            false,
            "",
            Json{{"start", 1}, {"end", 100}, {"step", 1}, {"circle", true}}
        ),
        make_metadata(
            "regular_expression",
            {
                {"pattern", "Regular expression pattern."},
            },
            false,
            false,
            "",
            Json{{"pattern", "ORD-[A-Z]{3}-\\d{4}"}}
        ),
    };
    return catalog;
}

}  // namespace

const std::vector<GeneratorMetadata>& get_generator_catalog() {
    return build_catalog();
}

const GeneratorMetadata* find_generator_metadata(const std::string& name) {
    const auto& catalog = get_generator_catalog();
    const auto  it      = std::find_if(catalog.begin(), catalog.end(), [&name](const GeneratorMetadata& meta) {
        return meta.name == name;
    });
    if (it == catalog.end()) { return nullptr; }
    return &(*it);
}

Json build_project_template() {
    Json root;
    root["rows"]                 = 100;
    root["output_format"]        = "csv";
    root["null_value_string"]    = nullptr;
    root["table_name"]           = "generated_data";
    root["include_create_table"] = true;

    Json fields = Json::array();
    fields.push_back(
        Json{
            {"name", "id"},
            {"generator", "sequence"},
            {"config", Json{{"start", 1}, {"end", 1000}, {"step", 1}, {"circle", true}}},
        }
    );
    fields.push_back(
        Json{
            {"name", "company_name"},
            {"generator", "company_name"},
            {"config", Json{{"languages", Json::array({"English"})}, {"use_translation", false}}},
            {"data_linkage", "company:group_1"},
        }
    );
    fields.push_back(
        Json{
            {"name", "industry"},
            {"generator", "industry"},
            {"config", Json{{"languages", Json::array({"English"})}}},
            {"data_linkage", "company:group_1"},
        }
    );
    fields.push_back(
        Json{
            {"name", "email"},
            {"generator", "email"},
            {"config", Json{{"languages", Json::array({"English"})}, {"domains", Json::array({"example.com"})}}},
            {"unique", true},
        }
    );
    root["fields"] = fields;
    return root;
}

}  // namespace data_generator::cli
