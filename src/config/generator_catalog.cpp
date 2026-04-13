// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file generator_catalog.cpp

#include "config/generator_catalog.h"

#include <algorithm>
#include <utility>

namespace datagen::config {

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
    // Disabled from external use:
    // if (name == "integer" || name == "unsigned_integer" || name == "decimal" || name == "decimal_string") {
    //     return "number";
    // }
    if (name == "integer" || name == "decimal") { return "number"; }
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
    if (name == "boolean" || name == "sequence" || name == "regular_expression") { return "utility"; }
    return "";
}

std::vector<GeneratorMetadata> build_catalog() {
    std::vector catalog = {
        make_metadata(
            "company_name",
            {
                {"languages",
                 "array<string>",
                 "Select one or multiple languages.",
                 true,
                 {"English", "Simplified Chinese", "Traditional Chinese", "Japanese"}},
                {"use_translation", "boolean", "Decide whether to use translated output."},
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
                 "Select one or multiple languages.",
                 true,
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
                 "Select one or multiple languages.",
                 true,
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
                {"ip_address_type", "string", "Select an IP address type.", true, {"IPv4", "IPv6"}},
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
                 "Select one or multiple operating systems.",
                 true,
                 {"Windows", "macOS", "Linux"}},
                {"extensions", "array<string>", "Enter extension or list of extensions.", true},
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
                 "Select one or multiple operating systems.",
                 true,
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
                {"extensions", "array<string>", "Enter extension or list of extensions.", true},
            },
            false,
            true,
            "file",
            Json{{"extensions", Json::array({"txt", "csv"})}}
        ),
        make_metadata(
            "file_extension",
            {
                {"extensions", "array<string>", "Enter extension or list of extensions.", true},
            },
            false,
            true,
            "file",
            Json{{"extensions", Json::array({"txt", "csv"})}}
        ),
        make_metadata(
            "url",
            {
                {"subdomains", "array<string>", "Enter subdomains or list of subdomains.", true},
                {"tlds", "array<string>", "Enter top-level domains or list of top-level domains.", true},
            },
            true,
            false,
            "",
            Json{{"subdomains", Json::array({"www"})}, {"tlds", Json::array({"com", "net", "org"})}}
        ),
        make_metadata(
            "hostname",
            {
                {"subdomains", "array<string>", "Enter subdomains or list of subdomains.", true},
                {"tlds", "array<string>", "Enter top-level domains or list of top-level domains.", true},
            },
            true,
            false,
            "",
            Json{{"subdomains", Json::array({"www"})}, {"tlds", Json::array({"com", "net", "org"})}}
        ),
        make_metadata(
            "date",
            {
                {"start_date", "string", "Enter date in YYYY-MM-DD format.", true},
                {"end_date", "string", "Enter date in YYYY-MM-DD format.", true},
                {"days_of_week",
                 "array<string>",
                 "Select one or multiple days of week.",
                 true,
                 {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"}},
            },
            false,
            false,
            "",
            Json{
                {"start_date", "1970-01-01"},
                {"end_date", "2050-12-31"},
                {"days_of_week",
                 Json::array({"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"})},
            }
        ),
        make_metadata(
            "time",
            {
                {"start_time", "string", "Enter time in HH:MM:SS format.", true},
                {"end_time", "string", "Enter time in HH:MM:SS format.", true},
            },
            false,
            false,
            "",
            Json{{"start_time", "00:00:00"}, {"end_time", "23:59:59"}}
        ),
        make_metadata(
            "datetime",
            {
                {"start_date", "string", "Enter date in YYYY-MM-DD format.", true},
                {"end_date", "string", "Enter date in YYYY-MM-DD format.", true},
                {"start_time", "string", "Enter time in HH:MM:SS format.", true},
                {"end_time", "string", "Enter time in HH:MM:SS format.", true},
                {"days_of_week",
                 "array<string>",
                 "Select one or multiple days of week.",
                 true,
                 {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"}},
            },
            false,
            false,
            "",
            Json{
                {"start_date", "1970-01-01"},
                {"end_date", "2050-12-31"},
                {"start_time", "00:00:00"},
                {"end_time", "23:59:59"},
                {"days_of_week",
                 Json::array({"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"})},
            }
        ),
        make_metadata(
            "address_line1",
            {
                {"regions",
                 "array<string>",
                 "Select one or multiple regions.",
                 true,
                 {"United States", "United Kingdom", "China", "Japan"}},
                {"use_translation", "boolean", "Decide whether to use translated output."},
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
                 "Select one or multiple regions.",
                 true,
                 {"United States", "United Kingdom", "China", "Japan"}},
                {"use_translation", "boolean", "Decide whether to use translated output."},
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
                 "Select one or multiple regions.",
                 true,
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
                 "Select one or multiple regions.",
                 true,
                 {"United States", "United Kingdom", "China", "Japan"}},
                {"use_translation", "boolean", "Decide whether to use translated output."},
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
                 "Select one or multiple regions.",
                 true,
                 {"United States", "United Kingdom", "China", "Japan"}},
                {"use_translation", "boolean", "Decide whether to use translated output."},
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
                 "Select a country code standard.",
                 true,
                 {"None", "ISO_3166_1_alpha_2", "ISO_3166_1_alpha_3"}},
                {"languages",
                 "array<string>",
                 "Select one or multiple languages.",
                 true,
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
                {"start", "number", "Enter a number.", true},
                {"end", "number", "Enter a number.", true},
            },
            false,
            false,
            "",
            Json{{"start", 0}, {"end", 100}}
        ),
        // Disabled from external use:
        // make_metadata(
        //     "unsigned_integer",
        //     {
        //         {"start", "number", "Start value."},
        //         {"end", "number", "End value."},
        //     },
        //     false,
        //     false,
        //     "",
        //     Json{{"start", 1}, {"end", 100}}
        // ),
        // make_metadata(
        //     "decimal",
        //     {
        //         {"start", "number", "Start value."},
        //         {"end", "number", "End value."},
        //         {"decimal_places", "number", "Number of decimal places."},
        //     },
        //     false,
        //     false,
        //     "",
        //     Json{{"start", 1.0}, {"end", 100.0}, {"decimal_places", 2}}
        // ),
        make_metadata(
            "decimal",
            {
                {"start", "number", "Enter a number.", true},
                {"end", "number", "Enter a number..", true},
                {"decimal_places", "number", "Enter number of decimal places.", true},
            },
            false,
            false,
            "",
            Json{{"start", 0.0}, {"end", 100.0}, {"decimal_places", 2}}
        ),
        // Disabled from external use (renamed to "decimal"):
        // make_metadata(
        //     "decimal_string",
        //     {
        //         {"start", "number", "Start value."},
        //         {"end", "number", "End value."},
        //         {"decimal_places", "number", "Number of decimal places."},
        //     },
        //     false,
        //     false,
        //     "",
        //     Json{{"start", 1.0}, {"end", 100.0}, {"decimal_places", 2}}
        // ),
        make_metadata(
            "payment_method",
            {
                {"payment_methods", "array<string>", "Enter a payment method or list of payment methods.", true},
            },
            false,
            true,
            "card",
            Json{{"payment_methods", Json::array({"Credit Card", "PayPal", "Apple Pay"})}}
        ),
        make_metadata(
            "card_type",
            {
                {"languages",
                 "array<string>",
                 "Select one or multiple languages.",
                 true,
                 {"English", "Simplified Chinese", "Traditional Chinese", "Japanese"}},
                {"card_types",
                 "array<string>",
                 "Select one or multiple card types.",
                 true,
                 {"AmericanExpress", "JCB", "MasterCard", "UnionPay", "Visa"}},
            },
            false,
            true,
            "card",
            Json{
                {"languages", Json::array({"English"})},
                {"card_types", Json::array({"AmericanExpress", "JCB", "MasterCard", "UnionPay", "Visa"})},
            }
        ),
        make_metadata(
            "card_number",
            {
                {"card_types",
                 "array<string>",
                 "Select one or multiple card types.",
                 true,
                 {"AmericanExpress", "JCB", "MasterCard", "UnionPay", "Visa"}},
            },
            true,
            true,
            "card",
            Json{{"card_types", Json::array({"AmericanExpress", "JCB", "MasterCard", "UnionPay", "Visa"})}}
        ),
        make_metadata(
            "card_date",
            {
                {"start_month", "string", "Enter date in MM/YY format.", true},
                {"end_month", "string", "Enter date in MM/YY format.", true},
            },
            false,
            true,
            "card",
            Json{{"start_month", "01/00"}, {"end_month", "12/50"}}
        ),
        make_metadata(
            "first_name",
            {
                {"languages",
                 "array<string>",
                 "Select one or multiple languages.",
                 true,
                 {"English", "Simplified Chinese", "Traditional Chinese", "Japanese"}},
                {"genders", "array<string>", "Select one or multiple genders.", true, {"M", "F"}},
                {"use_translation", "boolean", "Decide whether to use translated output."},
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
                 "Select one or multiple languages.",
                 true,
                 {"English", "Simplified Chinese", "Traditional Chinese", "Japanese"}},
                {"use_translation", "boolean", "Decide whether to use translated output."},
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
                 "Select one or multiple languages.",
                 true,
                 {"English", "Simplified Chinese", "Traditional Chinese", "Japanese"}},
                {"genders", "array<string>", "Select one or multiple genders.", true, {"M", "F"}},
                {"use_translation", "boolean", "Decide whether to use translated output."},
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
                 "Select one or multiple languages.",
                 true,
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
                 "Select one or multiple languages.",
                 true,
                 {"English", "Simplified Chinese", "Traditional Chinese", "Japanese"}},
                {"genders", "array<string>", "Select one or multiple genders.", true, {"M", "F"}},
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
                 "Select one or multiple languages.",
                 true,
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
                 "Select one or multiple regions.",
                 true,
                 {"United States", "United Kingdom", "China", "Japan"}},
                {"is_international", "boolean", "Decide whether to generate international numbers.", true},
                {"include_delimiters", "boolean", "Decide whether to include delimiters.", true},
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
                 "Select one or multiple languages.",
                 true,
                 {"English", "Simplified Chinese", "Traditional Chinese", "Japanese"}},
                {"domains", "array<string>", "Enter an email domain or list of email domains.", true},
            },
            true,
            true,
            "person",
            Json{{"languages", Json::array({"English"})}, {"domains", Json::array({"gmail.com", "outlook.com"})}}
        ),
        make_metadata(
            "job_title",
            {
                {"languages",
                 "array<string>",
                 "Select one or multiple languages.",
                 true,
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
                 "Select one or multiple languages.",
                 true,
                 {"English", "Simplified Chinese", "Traditional Chinese", "Japanese"}},
                {"use_translation", "boolean", "Decide whether to use translated output."},
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
                 "Select one or multiple languages.",
                 true,
                 {"English", "Simplified Chinese", "Traditional Chinese", "Japanese"}},
                {"keywords", "array<string>", "Enter a keyword or list of keywords.", true},
            },
            false,
            false,
            "",
            Json{
                {"languages", Json::array({"English"})},
                {"keywords",
                 Json::array({"Cherry", "Orange", "Pluots", "Grape", "Kiwi", "Mango", "Raspberry", "Strawberry"})},
            }
        ),
        make_metadata(
            "product_category",
            {
                {"languages",
                 "array<string>",
                 "Select one or multiple languages.",
                 true,
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
                 "Select one or multiple languages.",
                 true,
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
                 "Select one or multiple barcode types.",
                 true,
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
                {"enums", "array<string>", "Enter an item or list of items.", true},
            },
            false,
            false,
            "",
            Json{{"enums", Json::array({"First", "Second", "Third"})}}
        ),
        make_metadata(
            "text",
            {
                {"number_of_chars_start", "number", "Enter a number.", true},
                {"number_of_chars_end", "number", "Enter a number.", true},
            },
            false,
            false,
            "",
            Json{{"number_of_chars_start", 100}, {"number_of_chars_end", 10000}}
        ),
        make_metadata(
            "uuid",
            {
                {"include_hyphens", "boolean", "Decide whether to include hyphens.", true},
            },
            false,
            false,
            "",
            Json{{"include_hyphens", true}}
        ),
        make_metadata(
            "boolean",
            {
                {"true_percentage",
                 "number",
                 "Percentage of rows that return true. Value must be between 0 and 100.",
                 true},
            },
            false,
            false,
            "",
            Json{{"true_percentage", 50}}
        ),
        make_metadata(
            "sequence",
            {
                {"start", "number", "Enter a number.", true},
                {"end", "number", "Enter a number.", true},
                {"step", "number", "Enter a value greater than 0.", true},
                {"circle", "boolean", "Decide whether to wrap when reaching end.", true},
            },
            false,
            false,
            "",
            Json{{"start", 1}, {"end", 100}, {"step", 1}, {"circle", true}}
        ),
        make_metadata(
            "regular_expression",
            {
                {"pattern", "string", "Enter regular expression pattern.", true},
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

const std::vector<GeneratorMetadata> kGeneratorCatalog = build_catalog();

}  // namespace

const std::vector<GeneratorMetadata>& get_generator_catalog() {
    return kGeneratorCatalog;
}

const GeneratorMetadata* find_generator_metadata(const std::string& name) {
    const auto& catalog = get_generator_catalog();
    const auto  it      = std::find_if(catalog.begin(), catalog.end(), [&name](const GeneratorMetadata& meta) {
        return meta.name == name;
    });
    if (it == catalog.end()) { return nullptr; }
    return &(*it);
}

Json build_project_template(const int rows, const std::string& file_format) {
    Json root;
    root["rows"]   = rows;
    Json output    = Json::object();
    output["type"] = "file";
    Json file      = Json::object();
    file["format"] = file_format;
    Json options   = Json::object();
    if (file_format == "csv") {
        options["header"]      = true;
        options["line_ending"] = "LF";
    } else if (file_format == "json") {
        options["array"]        = true;
        options["include_null"] = true;
    } else if (file_format == "sql") {
        options["table"]        = "generated_data";
        options["create_table"] = false;
    } else if (file_format == "Tab-Delimited") {
        options["header"]      = true;
        options["line_ending"] = "LF";
    } else if (file_format == "Custom") {
        options["delimiter"]   = ",";
        options["quote"]       = "\"";
        options["header"]      = true;
        options["line_ending"] = "LF";
    }
    if (!options.empty()) { file["options"] = std::move(options); }
    output["file"] = std::move(file);
    root["output"] = std::move(output);

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
            {"group", "company:group_1"},
        }
    );
    fields.push_back(
        Json{
            {"name", "industry"},
            {"generator", "industry"},
            {"config", Json{{"languages", Json::array({"English"})}}},
            {"group", "company:group_1"},
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

}  // namespace datagen::config
