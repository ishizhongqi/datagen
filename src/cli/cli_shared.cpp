// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file cli_shared.cpp

#include "cli/cli_shared.h"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <unordered_set>
#include <utility>

namespace data_generator::cli {

namespace {

Json BuildSchemaForParamType(const std::string& param_type) {
    if (param_type == "string") { return Json{{"type", "string"}}; }
    if (param_type == "number") { return Json{{"type", "number"}}; }
    if (param_type == "boolean") { return Json{{"type", "boolean"}}; }
    if (param_type == "array<string>") {
        return Json{
            {"type", "array"},
            {"items", Json{{"type", "string"}}},
        };
    }

    // Fallback for unknown type hints: allow any JSON value.
    return Json::object();
}

Json BuildSchemaForConfigParam(const ConfigParam& param) {
    Json schema = BuildSchemaForParamType(param.type);
    if (param.supported_values.empty()) { return schema; }

    if (param.type == "array<string>") {
        if (!schema.contains("items") || !schema.at("items").is_object()) { schema["items"] = Json::object(); }
        schema["items"]["enum"] = param.supported_values;
        return schema;
    }

    schema["enum"] = param.supported_values;
    return schema;
}

Json BuildConfigSchema(const GeneratorMetadata& meta) {
    Json config_schema;
    config_schema["type"]                 = "object";
    config_schema["properties"]           = Json::object();
    config_schema["additionalProperties"] = false;

    Json required = Json::array();
    for (const auto& param : meta.config_params) {
        config_schema["properties"][param.name] = BuildSchemaForConfigParam(param);
        if (param.required) { required.push_back(param.name); }
    }
    if (!required.empty()) { config_schema["required"] = std::move(required); }

    return config_schema;
}

Json BuildGeneratorConstraint(const GeneratorMetadata& meta) {
    Json constraint;
    constraint["if"] = Json{
        {"properties", Json{{"generator", Json{{"const", meta.name}}}}},
        {"required", Json::array({"generator"})},
    };

    Json then_schema;
    then_schema["properties"] = Json{
        {"config", BuildConfigSchema(meta)},
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

}  // namespace

Json load_json_from_file(const std::string& path) {
    std::ifstream input_stream(path);
    if (!input_stream) { throw std::runtime_error("Failed to open input JSON: " + path); }

    Json root;
    try {
        input_stream >> root;
    } catch (const std::exception& ex) {
        throw std::runtime_error("Failed to parse JSON in " + path + ": " + std::string(ex.what()));
    }
    return root;
}

OrderedJson to_ordered_json(const Json& value) {
    if (value.is_object()) {
        OrderedJson object = OrderedJson::object();
        for (auto it = value.begin(); it != value.end(); ++it) { object[it.key()] = to_ordered_json(it.value()); }
        return object;
    }
    if (value.is_array()) {
        OrderedJson array = OrderedJson::array();
        for (const auto& item : value) { array.push_back(to_ordered_json(item)); }
        return array;
    }
    return OrderedJson(value);
}

OrderedJson build_ordered_config_template(const GeneratorMetadata& meta) {
    OrderedJson                ordered = OrderedJson::object();
    std::unordered_set<std::string> seen;
    seen.reserve(meta.config_params.size());

    for (const auto& param : meta.config_params) {
        if (meta.config_template.contains(param.name)) {
            ordered[param.name] = to_ordered_json(meta.config_template.at(param.name));
            seen.insert(param.name);
        }
    }
    for (auto it = meta.config_template.begin(); it != meta.config_template.end(); ++it) {
        if (!seen.contains(it.key())) { ordered[it.key()] = to_ordered_json(it.value()); }
    }
    return ordered;
}

nlohmann::json BuildJsonSchema() {
    const auto& catalog = get_generator_catalog();

    Json generator_names = Json::array();
    for (const auto& meta : catalog) { generator_names.push_back(meta.name); }

    Json field_constraints = Json::array();
    for (const auto& meta : catalog) { field_constraints.push_back(BuildGeneratorConstraint(meta)); }

    Json field_item_schema;
    field_item_schema["type"]                 = "object";
    field_item_schema["additionalProperties"] = false;
    field_item_schema["properties"] = Json{
        {"name", Json{{"type", "string"}}},
        {"generator", Json{{"type", "string"}, {"enum", generator_names}}},
        {"config", Json{{"type", "object"}}},
        {"unique", Json{{"type", "boolean"}}},
        {"nullable", Json{{"type", "boolean"}}},
        {"data_linkage", Json{{"type", "string"}}},
        {"supports_unique", Json{{"type", "boolean"}}},
        {"supports_data_linkage", Json{{"type", "boolean"}}},
        {"null_value", Json{{"type", "object"}}},
        {"default_value", Json{{"type", "object"}}},
    };
    field_item_schema["required"] = Json::array({"name", "generator", "config"});
    field_item_schema["allOf"]    = std::move(field_constraints);

    Json schema;
    schema["$schema"]              = "https://json-schema.org/draft/2020-12/schema";
    schema["type"]                 = "object";
    schema["additionalProperties"] = false;
    schema["patternProperties"]    = Json{{R"(^\$schema$)", Json{{"type", "string"}}}};
    schema["properties"] = Json{
        {"workspace", Json{{"type", "string"}, {"minLength", 1}}},
        {"rows", Json{{"type", "integer"}, {"minimum", 1}}},
        {"output_format", Json{{"type", "string"}, {"enum", Json::array({"csv", "json", "sql"})}}},
        {"output_dest", Json{{"type", "string"}, {"enum", Json::array({"file", "database"})}}},
        {"output_destination", Json{{"type", "string"}, {"enum", Json::array({"file", "database"})}}},
        {"output_path", Json{{"type", "string"}}},
        {"url", Json{{"type", "string"}, {"minLength", 1}}},
        {"database_url", Json{{"type", "string"}, {"minLength", 1}}},
        {"odbc_connection", Json{{"type", "string"}, {"minLength", 1}}},
        {"table", Json{{"type", "string"}, {"minLength", 1}}},
        {"database_table", Json{{"type", "string"}, {"minLength", 1}}},
        {"insert_mode", Json{{"type", "string"}, {"enum", Json::array({"auto", "insert", "bulk", "load"})}}},
        {"batch_size", Json{{"type", "integer"}, {"minimum", 1}}},
        {"queue_size", Json{{"type", "integer"}, {"minimum", 1}}},
        {"db_threads", Json{{"type", "integer"}, {"minimum", 1}}},
        {"transaction_mode", Json{{"type", "string"}, {"enum", Json::array({"per-batch", "per-run", "none"})}}},
        {"error_policy",
         Json{{"type", "string"}, {"enum", Json::array({"stop", "continue", "rollback-batch", "rollback-all"})}}},
        {"rate_limit_rows_per_sec", Json{{"type", "integer"}, {"minimum", 1}}},
        {"null_value_string", Json{{"type", Json::array({"string", "null"})}}},
        {"table_name", Json{{"type", "string"}, {"minLength", 1}}},
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

cxxopts::ParseResult parse_options(cxxopts::Options& options, const std::vector<std::string>& args) {
    std::vector<std::string> arg_storage;
    arg_storage.reserve(args.size() + 1);
    arg_storage.push_back(options.program());
    arg_storage.insert(arg_storage.end(), args.begin(), args.end());

    std::vector<char*> argv;
    argv.reserve(arg_storage.size());
    for (auto& arg : arg_storage) { argv.push_back(const_cast<char*>(arg.c_str())); }

    return options.parse(static_cast<int>(argv.size()), argv.data());
}

void print_validation_issues(const std::vector<core::ValidationIssue>& issues, std::ostream& output) {
    for (const auto& issue : issues) {
        output << (issue.warning ? "Validation warning: " : "Validation error: ")
               << issue.path << " " << issue.message << "\n";
    }
}

std::vector<std::string> build_describe_text_lines(const GeneratorMetadata& meta) {
    std::vector<std::string> lines;
    lines.emplace_back("Generator: " + meta.name);
    lines.emplace_back("");

    if (!meta.config_params.empty()) {
        lines.emplace_back("Config:");
        for (const auto& param : meta.config_params) {
            const std::string type_text   = param.type.empty() ? "unknown" : param.type;
            std::string       values_text = "any";
            if (type_text == "boolean") {
                values_text = "true, false";
            } else if (!param.supported_values.empty()) {
                values_text.clear();
                for (size_t i = 0; i < param.supported_values.size(); ++i) {
                    if (i > 0) { values_text += ", "; }
                    values_text += param.supported_values[i];
                }
            }
            lines.emplace_back("* " + param.name + " (type=" + type_text + ", values=" + values_text + ")");
        }
        lines.emplace_back("");
    }

    lines.emplace_back("Advanced Settings:");
    if (meta.supports_unique) { lines.emplace_back("* unique (type=boolean)"); }
    if (meta.supports_data_linkage) {
        const std::string module_hint = meta.linkage_module.empty()
                                            ? (meta.module.empty() ? "module" : meta.module)
                                            : meta.linkage_module;
        lines.emplace_back("* data_linkage (type=string, format: " + module_hint + ":Group1)");
    }
    lines.emplace_back("* null_value (type=object)");
    lines.emplace_back("* default_value (type=object)");
    lines.emplace_back("");

    if (!meta.module.empty()) {
        lines.emplace_back("Module: " + meta.module);
        lines.emplace_back("");
    }

    lines.emplace_back("Example:");
    OrderedJson example  = OrderedJson::object();
    example["name"]      = "field_1";
    example["generator"] = meta.name;
    example["config"]    = build_ordered_config_template(meta);
    if (meta.supports_unique) { example["unique"] = false; }
    if (meta.supports_data_linkage) {
        const std::string module_hint = meta.linkage_module.empty()
                                            ? (meta.module.empty() ? "module" : meta.module)
                                            : meta.linkage_module;
        example["data_linkage"]       = module_hint + ":Group1";
    }
    example["null_value"]    = OrderedJson{{"enabled", false}, {"percent", 0}};
    example["default_value"] = OrderedJson{{"enabled", false}, {"percent", 0}, {"value", ""}};

    std::istringstream stream(example.dump(2));
    std::string        line;
    while (std::getline(stream, line)) { lines.push_back(std::move(line)); }

    return lines;
}

}  // namespace data_generator::cli
