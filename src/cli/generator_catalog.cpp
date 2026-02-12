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
    meta.module                = "";
    meta.config_params         = std::move(params);
    meta.supports_unique       = supports_unique;
    meta.supports_data_linkage = supports_data_linkage;
    meta.linkage_module        = std::move(linkage_module);
    meta.config_template       = std::move(config_template);
    return meta;
}

std::string module_for_generator(const std::string& name) {
    if (name == "company_name" || name == "department" || name == "industry") { return "business"; }
    if (name ==
        "ip_address" ||
        name ==
        "mac_address" ||
        name ==
        "file_path" ||
        name ==
        "file_directory" ||
        name ==
        "file_name" ||
        name ==
        "file_extension" ||
        name ==
        "url" ||
        name == "hostname") {
        return "computer";
    }
    if (name == "date" || name == "time" || name == "datetime") { return "datetime"; }
    if (name ==
        "address_line1" ||
        name ==
        "address_line2" ||
        name ==
        "postcode" ||
        name ==
        "full_address" ||
        name ==
        "city" ||
        name == "region") {
        return "location";
    }
    if (name == "integer" || name == "unsigned_integer" || name == "decimal" || name == "decimal_string") {
        return "number";
    }
    if (name == "payment_method" || name == "card_type" || name == "card_number" || name == "card_date") {
        return "payment";
    }
    if (name ==
        "first_name" ||
        name ==
        "last_name" ||
        name ==
        "full_name" ||
        name ==
        "gender" ||
        name ==
        "title" ||
        name ==
        "marital_status" ||
        name ==
        "phone_number" ||
        name ==
        "email" ||
        name ==
        "job_title" ||
        name == "social_network_id") {
        return "person";
    }
    if (name ==
        "product_name" ||
        name ==
        "product_category" ||
        name ==
        "color" ||
        name ==
        "size" ||
        name == "barcode") {
        return "product";
    }
    if (name == "enum_item" || name == "text" || name == "uuid") { return "string"; }
    if (name == "sequence" || name == "regular_expression") { return "utility"; }
    return "";
}

std::vector<GeneratorMetadata> build_catalog() {
    std::vector catalog = {
        make_metadata(
            "company_name",
            {
                {"languages",
                 "array<string>",
                 "Language names (string or array).",
                 false,
                 {"English", "Simplified Chinese", "Traditional Chinese", "Japanese"}},
                {"use_translation", "boolean", "Whether to use translated output."},
            },
            false,
            true,
            "company",
            Json{{"languages", Json::array({"English"})}, {"use_translation", false}}
        ),
        make_metadata(
            "department",
            {
                {"languages",
                 "array<string>",
                 "Language names (string or array).",
                 false,
                 {"English", "Simplified Chinese", "Traditional Chinese", "Japanese"}},
            },
            false,
            false,
            "location",
            Json{{"languages", Json::array({"English"})}}
        ),
        make_metadata(
            "industry",
            {
                {"languages",
                 "array<string>",
                 "Language names (string or array).",
                 false,
                 {"English", "Simplified Chinese", "Traditional Chinese", "Japanese"}},
            },
            false,
            true,
            "company",
            Json{{"languages", Json::array({"English"})}}
        ),
        make_metadata(
            "ip_address",
            {
                {"ip_address_type", "string", "IPv4 or IPv6.", false, {"IPv4", "IPv6"}},
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
                {"operating_systems",
                 "array<string>",
                 "Operating systems (string or array).",
                 false,
                 {"Windows", "macOS", "Linux"}},
                {"extensions", "array<string>", "File extensions (string or array).", false},
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
                {"operating_systems",
                 "array<string>",
                 "Operating systems (string or array).",
                 false,
                 {"Windows", "macOS", "Linux"}},
            },
            false,
            true,
            "file",
            Json{{"operating_systems", Json::array({"Windows"})}}
        ),
        make_metadata(
            "file_name",
            {
                {"extensions", "array<string>", "File extensions (string or array).", false},
            },
            false,
            true,
            "file",
            Json{{"extensions", Json::array({"txt", "csv"})}}
        ),
        make_metadata(
            "file_extension",
            {
                {"extensions", "array<string>", "File extensions (string or array).", false},
            },
            false,
            true,
            "file",
            Json{{"extensions", Json::array({"txt", "csv"})}}
        ),
        make_metadata(
            "url",
            {
                {"subdomains", "array<string>", "Subdomains (string or array).", false},
                {"tlds", "array<string>", "Top-level domains (string or array).", true},
            },
            true,
            false,
            "",
            Json{{"subdomains", Json::array({"www"})}, {"tlds", Json::array({"com"})}}
        ),
        make_metadata(
            "hostname",
            {
                {"subdomains", "array<string>", "Subdomains (string or array).", false},
                {"tlds", "array<string>", "Top-level domains (string or array).", true},
            },
            true,
            false,
            "",
            Json{{"subdomains", Json::array({"www"})}, {"tlds", Json::array({"com"})}}
        ),
        make_metadata(
            "date",
            {
                {"start_date", "string", "Start date in YYYY-MM-DD.", false},
                {"end_date", "string", "End date in YYYY-MM-DD.", false},
                {"days_of_week",
                 "array<string>",
                 "Days to include (string or array).",
                 false,
                 {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"}},
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
                {"start_time", "string", "Start time in HH:MM:SS.", false},
                {"end_time", "string", "End time in HH:MM:SS.", false},
            },
            false,
            false,
            "",
            Json{{"start_time", "08:00:00"}, {"end_time", "17:00:00"}}
        ),
        make_metadata(
            "datetime",
            {
                {"start_date", "string", "Start date in YYYY-MM-DD.", false},
                {"end_date", "string", "End date in YYYY-MM-DD.", false},
                {"start_time", "string", "Start time in HH:MM:SS.", false},
                {"end_time", "string", "End time in HH:MM:SS.", false},
                {"days_of_week",
                 "array<string>",
                 "Days to include (string or array).",
                 false,
                 {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"}},
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
                {"regions",
                 "array<string>",
                 "Regions (string or array).",
                 false,
                 {"United States", "United Kingdom", "China", "Japan"}},
                {"use_translation", "boolean", "Whether to use translated output."},
            },
            false,
            true,
            "location",
            Json{{"regions", Json::array({"United States"})}, {"use_translation", false}}
        ),
        make_metadata(
            "address_line2",
            {
                {"regions",
                 "array<string>",
                 "Regions (string or array).",
                 false,
                 {"United States", "United Kingdom", "China", "Japan"}},
                {"use_translation", "boolean", "Whether to use translated output."},
            },
            false,
            true,
            "location",
            Json{{"regions", Json::array({"United States"})}, {"use_translation", false}}
        ),
        make_metadata(
            "postcode",
            {
                {"regions",
                 "array<string>",
                 "Regions (string or array).",
                 false,
                 {"United States", "United Kingdom", "China", "Japan"}},
            },
            false,
            true,
            "location",
            Json{{"regions", Json::array({"United States"})}}
        ),
        make_metadata(
            "full_address",
            {
                {"regions",
                 "array<string>",
                 "Regions (string or array).",
                 false,
                 {"United States", "United Kingdom", "China", "Japan"}},
                {"use_translation", "boolean", "Whether to use translated output."},
            },
            false,
            true,
            "location",
            Json{{"regions", Json::array({"United States"})}, {"use_translation", false}}
        ),
        make_metadata(
            "city",
            {
                {"regions",
                 "array<string>",
                 "Regions (string or array).",
                 false,
                 {"United States", "United Kingdom", "China", "Japan"}},
                {"use_translation", "boolean", "Whether to use translated output."},
            },
            false,
            true,
            "location",
            Json{{"regions", Json::array({"United States"})}, {"use_translation", false}}
        ),
        make_metadata(
            "region",
            {
                {"country_codes_standard",
                 "string",
                 "Country code standard string.",
                 false,
                 {"None", "ISO_3166_1_alpha_2", "ISO_3166_1_alpha_3"}},
                {"languages",
                 "array<string>",
                 "Language names (string or array).",
                 false,
                 {"English", "Simplified Chinese", "Traditional Chinese", "Japanese"}},
            },
            false,
            true,
            "location",
            Json{{"country_codes_standard", "None"}, {"languages", Json::array({"English"})}}
        ),
        make_metadata(
            "integer",
            {
                {"start", "number", "Start value."},
                {"end", "number", "End value."},
            },
            false,
            false,
            "",
            Json{{"start", 1}, {"end", 100}}
        ),
        make_metadata(
            "unsigned_integer",
            {
                {"start", "number", "Start value."},
                {"end", "number", "End value."},
            },
            false,
            false,
            "",
            Json{{"start", 1}, {"end", 100}}
        ),
        make_metadata(
            "decimal",
            {
                {"start", "number", "Start value."},
                {"end", "number", "End value."},
                {"decimal_places", "number", "Number of decimal places."},
            },
            false,
            false,
            "",
            Json{{"start", 1.0}, {"end", 100.0}, {"decimal_places", 2}}
        ),
        make_metadata(
            "decimal_string",
            {
                {"start", "number", "Start value."},
                {"end", "number", "End value."},
                {"decimal_places", "number", "Number of decimal places."},
            },
            false,
            false,
            "",
            Json{{"start", 1.0}, {"end", 100.0}, {"decimal_places", 2}}
        ),
        make_metadata(
            "payment_method",
            {
                {"payment_methods", "array<string>", "Payment methods (string or array).", false},
            },
            false,
            true,
            "card",
            Json{{"payment_methods", Json::array({"Credit Card", "PayPal"})}}
        ),
        make_metadata(
            "card_type",
            {
                {"languages",
                 "array<string>",
                 "Language names (string or array).",
                 false,
                 {"English", "Simplified Chinese", "Traditional Chinese", "Japanese"}},
                {"card_types",
                 "array<string>",
                 "Card types (string or array).",
                 false,
                 {"AmericanExpress", "JCB", "MasterCard", "UnionPay", "Visa"}},
            },
            false,
            true,
            "card",
            Json{{"languages", Json::array({"English"})}, {"card_types", Json::array({"Visa", "MasterCard"})}}
        ),
        make_metadata(
            "card_number",
            {
                {"card_types",
                 "array<string>",
                 "Card types (string or array).",
                 false,
                 {"AmericanExpress", "JCB", "MasterCard", "UnionPay", "Visa"}},
            },
            true,
            true,
            "card",
            Json{{"card_types", Json::array({"Visa", "MasterCard"})}}
        ),
        make_metadata(
            "card_date",
            {
                {"start_month", "string", "Start date in MM/YY.", false},
                {"end_month", "string", "End date in MM/YY.", false},
            },
            false,
            true,
            "card",
            Json{{"start", "01/23"}, {"end", "12/30"}}
        ),
        make_metadata(
            "first_name",
            {
                {"languages",
                 "array<string>",
                 "Language names (string or array).",
                 false,
                 {"English", "Simplified Chinese", "Traditional Chinese", "Japanese"}},
                {"genders", "array<string>", "Genders (string or array).", false, {"M", "F", "Male", "Female"}},
                {"use_translation", "boolean", "Whether to use translated output."},
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
                {"languages",
                 "array<string>",
                 "Language names (string or array).",
                 false,
                 {"English", "Simplified Chinese", "Traditional Chinese", "Japanese"}},
                {"use_translation", "boolean", "Whether to use translated output."},
            },
            false,
            true,
            "person",
            Json{{"languages", Json::array({"English"})}, {"use_translation", false}}
        ),
        make_metadata(
            "full_name",
            {
                {"languages",
                 "array<string>",
                 "Language names (string or array).",
                 false,
                 {"English", "Simplified Chinese", "Traditional Chinese", "Japanese"}},
                {"genders", "array<string>", "Genders (string or array).", false, {"M", "F", "Male", "Female"}},
                {"use_translation", "boolean", "Whether to use translated output."},
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
                {"languages",
                 "array<string>",
                 "Language names (string or array).",
                 false,
                 {"English", "Simplified Chinese", "Traditional Chinese", "Japanese"}},
            },
            false,
            true,
            "person",
            Json{{"languages", Json::array({"English"})}}
        ),
        make_metadata(
            "title",
            {
                {"languages",
                 "array<string>",
                 "Language names (string or array).",
                 false,
                 {"English", "Simplified Chinese", "Traditional Chinese", "Japanese"}},
                {"genders", "array<string>", "Genders (string or array).", false, {"M", "F", "Male", "Female"}},
            },
            false,
            true,
            "person",
            Json{{"languages", Json::array({"English"})}, {"genders", Json::array({"M", "F"})}}
        ),
        make_metadata(
            "marital_status",
            {
                {"languages",
                 "array<string>",
                 "Language names (string or array).",
                 false,
                 {"English", "Simplified Chinese", "Traditional Chinese", "Japanese"}},
            },
            false,
            true,
            "person",
            Json{{"languages", Json::array({"English"})}}
        ),
        make_metadata(
            "phone_number",
            {
                {"regions",
                 "array<string>",
                 "Regions (string or array).",
                 false,
                 {"United States", "United Kingdom", "China", "Japan"}},
                {"is_international", "boolean", "Whether to generate international numbers."},
                {"include_delimiters", "boolean", "Whether to include delimiters."},
            },
            true,
            true,
            "person",
            Json{{"regions", Json::array({"United States"})}, {"is_international", false}, {"include_delimiters", true}}
        ),
        make_metadata(
            "email",
            {
                {"languages",
                 "array<string>",
                 "Language names (string or array).",
                 false,
                 {"English", "Simplified Chinese", "Traditional Chinese", "Japanese"}},
                {"domains", "array<string>", "Email domains (string or array).", false},
            },
            true,
            true,
            "person",
            Json{{"languages", Json::array({"English"})}, {"domains", Json::array({"example.com"})}}
        ),
        make_metadata(
            "job_title",
            {
                {"languages",
                 "array<string>",
                 "Language names (string or array).",
                 false,
                 {"English", "Simplified Chinese", "Traditional Chinese", "Japanese"}},
            },
            false,
            true,
            "person",
            Json{{"languages", Json::array({"English"})}}
        ),
        make_metadata(
            "social_network_id",
            {
                {"languages",
                 "array<string>",
                 "Language names (string or array).",
                 false,
                 {"English", "Simplified Chinese", "Traditional Chinese", "Japanese"}},
                {"use_translation", "boolean", "Whether to use translated output."},
            },
            true,
            true,
            "person",
            Json{{"languages", Json::array({"English"})}, {"use_translation", false}}
        ),
        make_metadata(
            "product_name",
            {
                {"languages",
                 "array<string>",
                 "Language names (string or array).",
                 false,
                 {"English", "Simplified Chinese", "Traditional Chinese", "Japanese"}},
                {"keywords", "array<string>", "Keyword list (string or array).", false},
            },
            false,
            false,
            "",
            Json{{"languages", Json::array({"English"})}, {"keywords", Json::array({"Cherry", "Orange"})}}
        ),
        make_metadata(
            "product_category",
            {
                {"languages",
                 "array<string>",
                 "Language names (string or array).",
                 false,
                 {"English", "Simplified Chinese", "Traditional Chinese", "Japanese"}},
            },
            false,
            false,
            "",
            Json{{"languages", Json::array({"English"})}}
        ),
        make_metadata(
            "color",
            {
                {"languages",
                 "array<string>",
                 "Language names (string or array).",
                 false,
                 {"English", "Simplified Chinese", "Traditional Chinese", "Japanese"}},
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
                {"barcode_types",
                 "array<string>",
                 "Barcode types (string or array).",
                 false,
                 {"EAN8", "EAN13", "UPCA", "UPCE", "ISBN"}},
            },
            true,
            false,
            "",
            Json{{"barcode_types", Json::array({"EAN13"})}}
        ),
        make_metadata(
            "enum_item",
            {
                {"enums", "array<string>", "Enum values (string or array).", true},
            },
            false,
            false,
            "",
            Json{{"enums", Json::array({"First", "Second", "Third"})}}
        ),
        make_metadata(
            "text",
            {
                {"number_of_chars_start", "number", "Minimum number of characters."},
                {"number_of_chars_end", "number", "Maximum number of characters."},
            },
            false,
            false,
            "",
            Json{{"number_of_chars_start", 100}, {"number_of_chars_end", 1000}}
        ),
        make_metadata(
            "uuid",
            {
                {"include_hyphens", "boolean", "Whether to include hyphens."},
            },
            false,
            false,
            "",
            Json{{"include_hyphens", true}}
        ),
        make_metadata(
            "sequence",
            {
                {"start", "number", "Start value."},
                {"end", "number", "End value."},
                {"step", "number", "Step increment (non-zero)."},
                {"circle", "boolean", "Whether to wrap when reaching end."},
            },
            false,
            false,
            "",
            Json{{"start", 1}, {"end", 100}, {"step", 1}, {"circle", true}}
        ),
        make_metadata(
            "regular_expression",
            {
                {"pattern", "string", "Regular expression pattern.", true},
            },
            false,
            false,
            "",
            Json{{"pattern", "ORD-[A-Z]{3}-\\d{4}"}}
        ),
    };
    for (auto& meta : catalog) { meta.module = module_for_generator(meta.name); }
    return catalog;
}

}  // namespace

const std::vector<GeneratorMetadata>& get_generator_catalog() {
    static const std::vector<GeneratorMetadata> catalog = build_catalog();
    return catalog;
}

const GeneratorMetadata* find_generator_metadata(const std::string& name) {
    const auto& catalog = get_generator_catalog();
    const auto  it      = std::find_if(catalog.begin(), catalog.end(), [&name](const GeneratorMetadata& meta) {
        return meta.name == name;
    });
    if (it == catalog.end()) { return nullptr; }
    return &(*it);
}

Json build_project_template(const int rows, const std::string& output_format) {
    Json root;
    root["rows"]                 = rows;
    root["output_format"]        = output_format;
    root["null_value_string"]    = nullptr;
    if (output_format == "sql") {
        root["table_name"] = "generated_data";
        root["include_create_table"] = true;
    }

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
