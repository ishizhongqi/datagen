// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file command_info.cpp

#include "cli/command_info.h"

#include <cxxopts.hpp>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "cli/cli_shared.h"
#include "cli/exit_codes.h"
#include "config/generator_catalog.h"

namespace data_generator::cli {

namespace {

void print_generator_list_text(const std::vector<config::GeneratorMetadata>& catalog) {
    std::vector<std::pair<std::string, std::vector<std::string>>> grouped;
    std::unordered_map<std::string, std::size_t>                  module_to_index;
    grouped.reserve(catalog.size());
    module_to_index.reserve(catalog.size());

    for (const auto& meta : catalog) {
        const auto it = module_to_index.find(meta.module);
        if (it == module_to_index.end()) {
            const std::size_t index = grouped.size();
            grouped.emplace_back(meta.module, std::vector{meta.name});
            module_to_index.emplace(meta.module, index);
        } else {
            grouped[it->second].second.push_back(meta.name);
        }
    }

    std::cout << "Available Generators:\n";
    bool first_group = true;
    for (const auto& [module, names] : grouped) {
        if (!first_group) { std::cout << "\n"; }
        first_group = false;
        std::cout << "[" << module << "]\n";
        for (const auto& name : names) { std::cout << "- " << name << "\n"; }
    }
}

void print_generator_list_json(const std::vector<config::GeneratorMetadata>& catalog) {
    OrderedJson output = OrderedJson::object();
    OrderedJson names  = OrderedJson::array();
    for (const auto& meta : catalog) { names.push_back(meta.name); }
    output["generators"] = std::move(names);
    std::cout << output.dump(2) << "\n";
}

void print_generator_detail_json(const config::GeneratorMetadata& meta) {
    OrderedJson output  = OrderedJson::object();
    output["generator"] = meta.name;
    output["module"]    = meta.module.empty() ? OrderedJson(nullptr) : OrderedJson(meta.module);
    output["supports"]  = OrderedJson{{"unique", meta.supports_unique}, {"data_linkage", meta.supports_data_linkage}};

    OrderedJson fields = OrderedJson::array();
    for (const auto& param : meta.config_params) {
        OrderedJson default_value = nullptr;
        if (meta.config_template.contains(param.name)) {
            default_value = to_ordered_json(meta.config_template.at(param.name));
        }
        const std::string type = param.type.empty() ? "unknown" : param.type;
        fields.push_back(
            OrderedJson{
                {"name", param.name},
                {"type", type},
                {"required", param.required},
                {"description", param.description},
                {"supported_values", to_ordered_json(param.supported_values)},
                {"default", default_value},
            }
        );
    }

    output["config"] = OrderedJson{{"fields", fields}, {"template", build_ordered_config_template(meta)}};

    std::cout << output.dump(2) << "\n";
}

std::vector<std::string> build_describe_text_lines(const config::GeneratorMetadata& meta) {
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
                for (std::string::size_type i = 0; i < param.supported_values.size(); ++i) {
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
        const std::string module_hint =
            meta.linkage_module.empty() ? (meta.module.empty() ? "module" : meta.module) : meta.linkage_module;
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
        const std::string module_hint =
            meta.linkage_module.empty() ? (meta.module.empty() ? "module" : meta.module) : meta.linkage_module;
        example["data_linkage"] = module_hint + ":Group1";
    }
    example["null_value"]    = OrderedJson{{"enabled", false}, {"percentage", 0}};
    example["default_value"] = OrderedJson{{"enabled", false}, {"percentage", 0}, {"value", ""}};

    std::istringstream stream(example.dump(2));
    std::string        line;
    while (std::getline(stream, line)) { lines.push_back(std::move(line)); }

    return lines;
}

}  // namespace

int CommandInfo::run(const std::vector<std::string>& args) {
    cxxopts::Options options("data-generator info", "List or describe generator metadata.");
    options.add_options()("name", "Generator name", cxxopts::value<std::string>())("json", "Output JSON")(
        "h,help",
        "Show help"
    );
    options.parse_positional({"name"});

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

    const auto& catalog = config::get_generator_catalog();

    if (!result.count("name")) {
        if (result.count("json")) {
            print_generator_list_json(catalog);
        } else {
            print_generator_list_text(catalog);
        }
        return exit_codes::kOk;
    }

    const std::string                generator_name = result["name"].as<std::string>();
    const config::GeneratorMetadata* meta           = config::find_generator_metadata(generator_name);
    if (!meta) {
        std::cerr << "Unknown generator: " << generator_name << "\n";
        return exit_codes::kUsage;
    }

    if (result.count("json")) {
        print_generator_detail_json(*meta);
        return exit_codes::kOk;
    }

    const auto lines = build_describe_text_lines(*meta);
    for (const auto& line : lines) { std::cout << line << "\n"; }
    return exit_codes::kOk;
}

}  // namespace data_generator::cli
