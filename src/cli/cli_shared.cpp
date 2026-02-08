// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file cli_shared.cpp

#include "cli/cli_shared.h"

#include <fstream>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

#include "cli/generator_catalog.h"
#include "generators/business_generators.h"
#include "generators/computer_generators.h"
#include "generators/datetime_generators.h"
#include "generators/linkage_helper.h"
#include "generators/location_generators.h"
#include "generators/number_generators.h"
#include "generators/payment_generators.h"
#include "generators/person_generators.h"
#include "generators/product_generators.h"
#include "generators/string_generators.h"
#include "generators/utility_generators.h"

namespace data_generator::cli {

namespace {

constexpr int         kDefaultRows = 100;
constexpr const char* kFieldsKey   = "fields";

}  // namespace

generator::GeneratorRegistry build_registry() {
    generator::GeneratorRegistry registry;
    register_computer_generators(registry);
    register_business_generators(registry);
    register_datetime_generators(registry);
    register_location_generators(registry);
    register_number_generators(registry);
    register_payment_generators(registry);
    register_person_generators(registry);
    register_product_generators(registry);
    register_string_generators(registry);
    register_utility_generators(registry);
    return registry;
}

Json load_json_from_file(const std::string& path) {
    std::ifstream input_stream(path);
    if (!input_stream) { throw std::runtime_error("Failed to open input json: " + path); }
    Json root;
    input_stream >> root;
    return root;
}

int resolve_row_count(const Json& root, int cli_rows, int default_rows) {
    if (cli_rows > 0) { return cli_rows; }
    if (root.contains("rows") && root.at("rows").is_number_integer()) { return root.at("rows").get<int>(); }
    return default_rows > 0 ? default_rows : kDefaultRows;
}

std::vector<FieldGenerator> build_field_generators(const Json& root, int rows) {
    if (!root.contains(kFieldsKey) || !root.at(kFieldsKey).is_array()) {
        throw std::runtime_error("Input json must contain array field: fields");
    }

    const generator::GeneratorRegistry registry = build_registry();
    std::vector<FieldGenerator>        fields;

    for (const auto& field : root.at(kFieldsKey)) {
        if (!field.is_object()) { throw std::runtime_error("Each field must be a json object"); }
        if (!field.contains("name") || !field.at("name").is_string()) {
            throw std::runtime_error("Each field must contain a string name");
        }
        if (!field.contains("generator") || !field.at("generator").is_string()) {
            throw std::runtime_error("Each field must contain a string generator");
        }
        if (!field.contains("config") || !field.at("config").is_object()) {
            throw std::runtime_error("Each field must contain object config");
        }

        Json field_with_rows    = field;
        field_with_rows["rows"] = rows;
        if (root.contains("null_value_string")) { field_with_rows["null_value_string"] = root["null_value_string"]; }

        const std::string name           = field.at("name").get<std::string>();
        const std::string generator_name = field.at("generator").get<std::string>();
        auto              generator      = registry.create(generator_name, field_with_rows);
        fields.push_back({name, std::move(generator)});
    }

    return fields;
}

std::string csv_escape(const std::string& value) {
    if (value.find_first_of(",\"\n\r") == std::string::npos) { return value; }
    std::string escaped;
    escaped.reserve(value.size() + 2);
    escaped.push_back('"');
    for (const char ch : value) {
        if (ch == '"') { escaped.push_back('"'); }
        escaped.push_back(ch);
    }
    escaped.push_back('"');
    return escaped;
}

std::string sql_escape(const std::string& value) {
    std::string escaped;
    escaped.reserve(value.size());
    for (const char ch : value) {
        if (ch == '\'') { escaped.push_back('\''); }
        escaped.push_back(ch);
    }
    return escaped;
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

bool validate_config(const Json& root, bool require_output_settings, std::vector<std::string>* errors) {
    if (!errors) { return false; }
    errors->clear();

    if (!root.is_object()) { errors->emplace_back("Root must be a json object."); }

    const bool has_rows = root.contains("rows");
    if (has_rows && !root.at("rows").is_number_integer()) {
        errors->emplace_back("rows must be an integer.");
    } else if (require_output_settings) {
        if (!has_rows) {
            errors->emplace_back("rows is required.");
        } else if (root.at("rows").get<int>() <= 0) {
            errors->emplace_back("rows must be a positive integer.");
        }
    }

    const bool has_output_format = root.contains("output_format");
    if (has_output_format && !root.at("output_format").is_string()) {
        errors->emplace_back("output_format must be a string.");
    } else if (has_output_format) {
        const std::string value = root.at("output_format").get<std::string>();
        if (value != "csv" && value != "json" && value != "sql") {
            errors->emplace_back("output_format must be one of csv, json, sql.");
        }
    } else if (require_output_settings) {
        errors->emplace_back("output_format is required.");
    }

    if (!root.contains("fields") || !root.at("fields").is_array()) { errors->emplace_back("fields must be an array."); }

    if (!errors->empty()) { return false; }

    const auto& fields = root.at("fields");
    for (size_t index = 0; index < fields.size(); ++index) {
        const auto& field = fields.at(index);
        if (!field.is_object()) {
            errors->emplace_back("fields[" + std::to_string(index) + "] must be an object.");
            continue;
        }

        std::string field_name;
        if (field.contains("name") && field.at("name").is_string()) {
            field_name = field.at("name").get<std::string>();
        }
        const std::string label = field_name.empty()
                                      ? "fields[" + std::to_string(index) + "]"
                                      : "fields[" + std::to_string(index) + "] (name: " + field_name + ")";

        std::vector<std::string> field_errors;
        if (!field.contains("name") || !field.at("name").is_string()) {
            field_errors.emplace_back(label + ".name must be a string.");
        }
        if (!field.contains("generator") || !field.at("generator").is_string()) {
            field_errors.emplace_back(label + ".generator must be a string.");
        }
        if (!field.contains("config") || !field.at("config").is_object()) {
            field_errors.emplace_back(label + ".config must be an object.");
        }

        const GeneratorMetadata* meta = nullptr;
        if (field.contains("generator") && field.at("generator").is_string()) {
            const std::string generator_name = field.at("generator").get<std::string>();
            meta                             = find_generator_metadata(generator_name);
            if (!meta) { field_errors.emplace_back(label + ": unknown generator " + generator_name); }
        }

        if (meta && field.contains("config") && field.at("config").is_object()) {
            if (field.contains("unique")) {
                if (!field.at("unique").is_boolean()) {
                    field_errors.emplace_back(label + ".unique must be a boolean.");
                } else if (!meta->supports_unique) {
                    field_errors.emplace_back(label + ": generator does not support unique.");
                }
            }

            if (field.contains("data_linkage")) {
                try {
                    generator::parse_linkage_key(field);
                } catch (const std::exception& ex) {
                    field_errors.emplace_back(label + ".data_linkage invalid: " + std::string(ex.what()));
                }
                if (!meta->supports_data_linkage) {
                    field_errors.emplace_back(label + ": generator does not support data_linkage.");
                }
            }

            std::set<std::string> allowed_keys;
            for (const auto& param : meta->config_params) { allowed_keys.insert(param.name); }
            for (auto it = field.at("config").begin(); it != field.at("config").end(); ++it) {
                if (allowed_keys.find(it.key()) == allowed_keys.end()) {
                    field_errors.emplace_back(label + ".config contains unknown key: " + it.key());
                }
            }

            for (const auto& param : meta->config_params) {
                if (param.required && !field.at("config").contains(param.name)) {
                    field_errors.emplace_back(label + ".config missing required key: " + param.name);
                }
            }
        }

        if (meta && !field_errors.empty()) {
            field_errors.emplace_back(label + ": describe output:");
            const auto describe_lines = build_describe_text_lines(*meta);
            for (const auto& line : describe_lines) {
                if (line.empty()) { continue; }
                field_errors.emplace_back(label + ": " + line);
            }
        }

        errors->insert(errors->end(), field_errors.begin(), field_errors.end());
    }

    if (!errors->empty()) { return false; }

    try {
        const int rows = resolve_row_count(root, 0, kDefaultRows);
        build_field_generators(root, rows);
    } catch (const std::exception& ex) {
        errors->emplace_back(ex.what());
        return false;
    }

    return true;
}

void print_validation_errors(const std::vector<std::string>& errors, std::ostream& output) {
    for (const auto& error : errors) { output << "Validation error: " << error << "\n"; }
}

std::vector<std::string> build_describe_text_lines(const GeneratorMetadata& meta) {
    std::vector<std::string> lines;
    lines.emplace_back("Generator: " + meta.name);
    lines.emplace_back("");

    if (!meta.config_params.empty()) {
        lines.emplace_back("Config (required):");
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

    lines.emplace_back("Advanced Settings (optional):");
    if (meta.supports_unique) { lines.emplace_back("* unique (type=boolean, default=true)"); }
    if (meta.supports_data_linkage) {
        const std::string module_hint = meta.module.empty() ? "module" : meta.module;
        lines.emplace_back(
            R"(* data_linkage (type=string, default="", suggested format: ")" + module_hint + R"(:group1"))"
        );
    }
    lines.emplace_back(R"(* null_value (type=object, default={"enabled":false,"percent":0}))");
    lines.emplace_back(R"(* default_value (type=object, default={"enabled":false,"percent":0,"value":""}))");
    lines.emplace_back("");

    if (!meta.module.empty()) { lines.emplace_back("Module: " + meta.module); }
    lines.emplace_back("");
    lines.emplace_back("Example:");
    Json example         = Json::object();
    example["name"]      = "field_1";
    example["generator"] = meta.name;
    example["config"]    = meta.config_template;
    if (meta.supports_unique) { example["unique"] = true; }
    if (meta.supports_data_linkage) {
        const std::string module_hint = meta.module.empty() ? "module" : meta.module;
        example["data_linkage"]       = module_hint + ":group1";
    }
    example["null_value"]    = Json{{"enabled", false}, {"percent", 0}};
    example["default_value"] = Json{{"enabled", false}, {"percent", 0}, {"value", ""}};

    const std::string  example_text = example.dump(2);
    std::istringstream stream(example_text);
    std::string        line;
    while (std::getline(stream, line)) { lines.emplace_back(line); }

    return lines;
}

}  // namespace data_generator::cli
