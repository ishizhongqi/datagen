// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file command_schema.cpp

#include "cli/command_schema.h"

#include <cxxopts.hpp>
#include <fstream>
#include <iostream>
#include <string>

#include "cli/cli_shared.h"
#include "cli/exit_codes.h"
#include "config/generator_catalog.h"

namespace data_generator::cli {

namespace {

using Json = nlohmann::json;

Json build_schema_for_param_type(const std::string& param_type) {
    if (param_type == "string") { return Json{{"type", "string"}}; }
    if (param_type == "number") { return Json{{"type", "number"}}; }
    if (param_type == "boolean") { return Json{{"type", "boolean"}}; }
    if (param_type == "array<string>") {
        return Json{
            {"type", "array"},
            {"items", Json{{"type", "string"}}},
        };
    }

    return Json::object();
}

Json build_param_for_config_param(const config::ConfigParam& param) {
    Json schema = build_schema_for_param_type(param.type);
    if (param.supported_values.empty()) { return schema; }

    if (param.type == "array<string>") {
        if (!schema.contains("items") || !schema.at("items").is_object()) { schema["items"] = Json::object(); }
        schema["items"]["enum"] = param.supported_values;
        return schema;
    }

    schema["enum"] = param.supported_values;
    return schema;
}

Json build_config_schema(const config::GeneratorMetadata& meta) {
    Json config_schema;
    config_schema["type"] = "object";
    config_schema["properties"] = Json::object();
    config_schema["additionalProperties"] = false;

    Json required = Json::array();
    for (const auto& param : meta.config_params) {
        config_schema["properties"][param.name] = build_param_for_config_param(param);
        if (param.required) { required.push_back(param.name); }
    }
    if (!required.empty()) { config_schema["required"] = std::move(required); }

    return config_schema;
}

Json build_generator_constraint(const config::GeneratorMetadata& meta) {
    Json constraint;
    constraint["if"] = Json{
        {"properties", Json{{"generator", Json{{"const", meta.name}}}}},
        {"required", Json::array({"generator"})},
    };

    Json then_schema;
    then_schema["properties"] = Json{
        {"config", build_config_schema(meta)},
    };

    Json restriction_rules = Json::array();
    if (!meta.supports_unique) {
        restriction_rules.push_back(Json{{"not", Json{{"required", Json::array({"unique"})}}}});
        restriction_rules.push_back(Json{{"not", Json{{"required", Json::array({"supports_unique"})}}}});
    }
    if (!meta.supports_data_linkage) {
        restriction_rules.push_back(Json{{"not", Json{{"required", Json::array({"data_linkage"})}}}});
        restriction_rules.push_back(Json{{"not", Json{{"required", Json::array({"supports_data_linkage"})}}}});
    }
    if (!restriction_rules.empty()) { then_schema["allOf"] = std::move(restriction_rules); }

    constraint["then"] = std::move(then_schema);
    return constraint;
}

Json build_json_schema() {
    const auto& catalog = config::get_generator_catalog();

    Json generator_names = Json::array();
    for (const auto& meta : catalog) { generator_names.push_back(meta.name); }

    Json field_constraints = Json::array();
    for (const auto& meta : catalog) { field_constraints.push_back(build_generator_constraint(meta)); }

    Json field_item_schema;
    field_item_schema["type"] = "object";
    field_item_schema["additionalProperties"] = false;
    field_item_schema["properties"] = Json{
        {"name", Json{{"type", "string"}}},
        {"generator", Json{{"type", "string"}, {"enum", generator_names}}},
        {"config", Json{{"type", "object"}}},
        {"unique", Json{{"type", "boolean"}}},
        {"data_linkage", Json{{"type", "string"}}},
        {"supports_unique", Json{{"type", "boolean"}}},
        {"supports_data_linkage", Json{{"type", "boolean"}}},
        {"null_value", Json{{"type", "object"}}},
        {"default_value", Json{{"type", "object"}}},
    };
    field_item_schema["required"] = Json::array({"name", "generator", "config"});
    field_item_schema["allOf"] = std::move(field_constraints);

    Json schema;
    schema["$schema"] = "https://json-schema.org/draft/2020-12/schema";
    schema["type"] = "object";
    schema["additionalProperties"] = false;
    schema["patternProperties"] = Json{{R"(^\$schema$)", Json{{"type", "string"}}}};
    Json csv_options = Json{
        {"type", "object"},
        {"additionalProperties", false},
        {"properties",
         Json{
             {"header", Json{{"type", "boolean"}}},
             {"line_ending", Json{{"type", "string"}, {"enum", Json::array({"LF", "CRLF"})}}},
         }},
    };
    Json json_options = Json{
        {"type", "object"},
        {"additionalProperties", false},
        {"properties",
         Json{
             {"array", Json{{"type", "boolean"}}},
             {"include_null", Json{{"type", "boolean"}}},
         }},
    };
    Json sql_options = Json{
        {"type", "object"},
        {"additionalProperties", false},
        {"properties",
         Json{
             {"table", Json{{"type", "string"}, {"minLength", 1}}},
             {"create_table", Json{{"type", "boolean"}}},
         }},
    };
    Json custom_options = Json{
        {"type", "object"},
        {"additionalProperties", false},
        {"properties",
         Json{
             {"delimiter", Json{{"type", "string"}, {"minLength", 1}}},
             {"quote", Json{{"type", "string"}}},
             {"header", Json{{"type", "boolean"}}},
             {"line_ending", Json{{"type", "string"}, {"enum", Json::array({"LF", "CRLF"})}}},
         }},
    };

    Json file_schema;
    file_schema["type"] = "object";
    file_schema["additionalProperties"] = false;
    file_schema["properties"] = Json{
        {"format", Json{{"type", "string"}, {"enum", Json::array({"csv", "json", "sql", "Tab-Delimited", "Custom"})}}},
        {"options", Json{{"type", "object"}}},
    };
    file_schema["required"] = Json::array({"format"});

    Json file_conditionals = Json::array();
    file_conditionals.push_back(
        Json{
            {"if", Json{{"properties", Json{{"format", Json{{"const", "csv"}}}}}, {"required", Json::array({"format"})}}},
            {"then", Json{{"properties", Json{{"options", csv_options}}}}},
        }
    );
    file_conditionals.push_back(
        Json{
            {"if", Json{{"properties", Json{{"format", Json{{"const", "Tab-Delimited"}}}}}, {"required", Json::array({"format"})}}},
            {"then", Json{{"properties", Json{{"options", csv_options}}}}},
        }
    );
    file_conditionals.push_back(
        Json{
            {"if", Json{{"properties", Json{{"format", Json{{"const", "json"}}}}}, {"required", Json::array({"format"})}}},
            {"then", Json{{"properties", Json{{"options", json_options}}}}},
        }
    );
    file_conditionals.push_back(
        Json{
            {"if", Json{{"properties", Json{{"format", Json{{"const", "sql"}}}}}, {"required", Json::array({"format"})}}},
            {"then", Json{{"properties", Json{{"options", sql_options}}}}},
        }
    );
    file_conditionals.push_back(
        Json{
            {"if", Json{{"properties", Json{{"format", Json{{"const", "Custom"}}}}}, {"required", Json::array({"format"})}}},
            {"then", Json{{"properties", Json{{"options", custom_options}}}}},
        }
    );
    file_schema["allOf"] = std::move(file_conditionals);

    Json database_schema;
    database_schema["type"] = "object";
    database_schema["additionalProperties"] = false;
    database_schema["properties"] = Json{
        {"connection", Json{{"type", "string"}, {"minLength", 1}}},
        {"table", Json{{"type", "string"}, {"minLength", 1}}},
        {"insert_mode", Json{{"type", "string"}, {"enum", Json::array({"auto", "insert", "bulk", "load"})}}},
        {"batch_size", Json{{"type", "integer"}, {"minimum", 1}}},
        {"queue_size", Json{{"type", "integer"}, {"minimum", 1}}},
        {"threads", Json{{"type", "integer"}, {"minimum", 1}}},
        {"transaction_mode", Json{{"type", "string"}, {"enum", Json::array({"per-batch", "per-run", "none"})}}},
        {"error_policy",
         Json{{"type", "string"}, {"enum", Json::array({"stop", "continue", "rollback-batch", "rollback-all"})}}},
        {"rate_limit_rows_per_sec", Json{{"type", "integer"}, {"minimum", 1}}},
    };
    database_schema["required"] = Json::array({"connection", "table"});

    Json output_schema;
    output_schema["type"] = "object";
    output_schema["additionalProperties"] = false;
    output_schema["properties"] = Json{
        {"type", Json{{"type", "string"}, {"enum", Json::array({"file", "database"})}}},
        {"file", std::move(file_schema)},
        {"database", std::move(database_schema)},
    };
    output_schema["allOf"] = Json::array({
        Json{
            {"if", Json{{"properties", Json{{"type", Json{{"const", "file"}}}}}, {"required", Json::array({"type"})}}},
            {"then", Json{{"required", Json::array({"file"})}}},
        },
        Json{
            {"if", Json{{"properties", Json{{"type", Json{{"const", "database"}}}}}, {"required", Json::array({"type"})}}},
            {"then", Json{{"required", Json::array({"database"})}}},
        },
    });

    schema["properties"] = Json{
        {"rows", Json{{"type", "integer"}, {"minimum", 1}}},
        {"output", std::move(output_schema)},
        {"fields",
         Json{
             {"type", "array"},
             {"minItems", 1},
             {"items", std::move(field_item_schema)},
         }},
    };
    schema["required"] = Json::array({"fields"});

    return schema;
}

}  // namespace

int CommandSchema::run(const std::vector<std::string>& args) {
    cxxopts::Options options("data-generator schema", "Generate JSON Schema from generator metadata.");
    options.add_options()
        ("output", "Output schema file path", cxxopts::value<std::string>())
        ("h,help", "Show help");
    options.parse_positional({"output"});

    cxxopts::ParseResult result;
    try {
        result = parse_options(options, args);
    } catch (const std::exception& ex) {
        std::cerr << "Failed to parse arguments: " << ex.what() << "\n";
        std::cerr << options.help() << "\n";
        return exit_codes::kUsage;
    }

    if (result.count("help")) {
        std::cout << options.help() << "\n";
        return exit_codes::kOk;
    }

    if (!result.count("output")) {
        std::cerr << "Missing required <file>\n";
        std::cerr << options.help() << "\n";
        return exit_codes::kUsage;
    }

    const std::string schema_text = build_json_schema().dump(2);
    const std::string path = result["output"].as<std::string>();
    std::ofstream     out(path, std::ios::trunc);
    if (!out) {
        std::cerr << "Failed to open output file: " << path << "\n";
        return exit_codes::kCliError;
    }
    out << schema_text << "\n";
    return exit_codes::kOk;
}

}  // namespace data_generator::cli
