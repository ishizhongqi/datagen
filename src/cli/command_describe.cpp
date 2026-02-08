// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file command_describe.cpp

#include "cli/command_describe.h"

#include <cxxopts.hpp>
#include <iostream>
#include <string>
#include <vector>

#include "cli/cli_shared.h"
#include "cli/generator_catalog.h"

namespace data_generator::cli {

namespace {

constexpr int kExitOk    = 0;
constexpr int kExitUsage = 2;

std::string infer_type(const Json& value) {
    if (value.is_string()) { return "string"; }
    if (value.is_boolean()) { return "boolean"; }
    if (value.is_number()) { return "number"; }
    if (value.is_object()) { return "object"; }
    if (value.is_array()) {
        if (value.empty()) { return "array"; }
        bool all_string = true;
        for (const auto& entry : value) {
            if (!entry.is_string()) {
                all_string = false;
                break;
            }
        }
        return all_string ? "array<string>" : "array";
    }
    return "null";
}

}  // namespace

int CommandDescribe::run(const std::vector<std::string>& args) {
    cxxopts::Options options("data-generator describe", "Describe generator metadata.");
    options.add_options()
        ("generator", "Generator name", cxxopts::value<std::string>())
        ("json", "Output JSON")
        ("h,help", "Show help");

    cxxopts::ParseResult result;
    try {
        result = parse_options(options, args);
    } catch (const std::exception& ex) {
        std::cerr << "Failed to parse arguments: " << ex.what() << "\n";
        std::cerr << options.help() << "\n";
        return kExitUsage;
    }

    if (result.count("help")) {
        std::cout << options.help() << "\n";
        return kExitOk;
    }

    if (!result.count("generator")) {
        std::cerr << "Missing required --generator\n";
        std::cerr << options.help() << "\n";
        return kExitUsage;
    }

    const std::string        generator_name = result["generator"].as<std::string>();
    const GeneratorMetadata* meta           = find_generator_metadata(generator_name);
    if (!meta) {
        std::cerr << "Unknown generator: " << generator_name << "\n";
        return kExitUsage;
    }

    if (!result.count("json")) {
        const auto lines = build_describe_text_lines(*meta);
        for (const auto& line : lines) { std::cout << line << "\n"; }
        return kExitOk;
    }

    Json output;
    output["generator"] = meta->name;
    output["module"] = meta->module.empty() ? Json(nullptr) : Json(meta->module);
    output["supports"] = Json{{"unique", meta->supports_unique}, {"data_linkage", meta->supports_data_linkage}};

    Json fields = Json::array();
    for (const auto& param : meta->config_params) {
        Json default_value = nullptr;
        if (meta->config_template.contains(param.name)) { default_value = meta->config_template.at(param.name); }
        const std::string type = param.type.empty() ? infer_type(default_value) : param.type;
        fields.push_back(
            Json{
                {"name", param.name},
                {"type", type},
                {"required", true},
                {"description", param.description},
                {"default", default_value},
            }
        );
    }

    output["config"] = Json{{"fields", fields}, {"template", meta->config_template}};

    std::cout << output.dump(2) << "\n";
    return kExitOk;
}

}  // namespace data_generator::cli
