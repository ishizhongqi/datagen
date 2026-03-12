// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file command_info.cpp

#include "cli/command_info.h"

#include <cxxopts.hpp>
#include <iostream>
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
    std::unordered_map<std::string, std::size_t>                 module_to_index;
    grouped.reserve(catalog.size());
    module_to_index.reserve(catalog.size());

    for (const auto& meta : catalog) {
        const auto it = module_to_index.find(meta.module);
        if (it == module_to_index.end()) {
            const std::size_t index = grouped.size();
            grouped.emplace_back(meta.module, std::vector<std::string>{meta.name});
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
    OrderedJson names = OrderedJson::array();
    for (const auto& meta : catalog) { names.push_back(meta.name); }
    output["generators"] = std::move(names);
    std::cout << output.dump(2) << "\n";
}

void print_generator_detail_json(const config::GeneratorMetadata& meta) {
    OrderedJson output = OrderedJson::object();
    output["generator"] = meta.name;
    output["module"] = meta.module.empty() ? OrderedJson(nullptr) : OrderedJson(meta.module);
    output["supports"] = OrderedJson{{"unique", meta.supports_unique}, {"data_linkage", meta.supports_data_linkage}};

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

}  // namespace

int CommandInfo::run(const std::vector<std::string>& args) {
    cxxopts::Options options("data-generator info", "List or describe generator metadata.");
    options.add_options()
        ("name", "Generator name", cxxopts::value<std::string>())
        ("json", "Output JSON")
        ("h,help", "Show help");
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

    const std::string generator_name = result["name"].as<std::string>();
    const config::GeneratorMetadata* meta = config::find_generator_metadata(generator_name);
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
