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
    for (const auto& issue : issues) { output << "Validation error: " << issue.path << " " << issue.message << "\n"; }
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
        const std::string module_hint = meta.module.empty() ? "module" : meta.module;
        lines.emplace_back("* data_linkage (type=string, format: " + module_hint + ":group_1)");
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
    if (meta.supports_unique) { example["unique"] = true; }
    if (meta.supports_data_linkage) {
        const std::string module_hint = meta.module.empty() ? "module" : meta.module;
        example["data_linkage"]       = module_hint + ":group_1";
    }
    example["null_value"]    = OrderedJson{{"enabled", false}, {"percent", 0}};
    example["default_value"] = OrderedJson{{"enabled", false}, {"percent", 0}, {"value", ""}};

    std::istringstream stream(example.dump(2));
    std::string        line;
    while (std::getline(stream, line)) { lines.push_back(std::move(line)); }

    return lines;
}

}  // namespace data_generator::cli
