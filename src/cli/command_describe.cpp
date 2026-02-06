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

}  // namespace

int CommandDescribe::run(const std::vector<std::string>& args) {
    cxxopts::Options options("data-generator describe", "Describe generator metadata.");
    options.add_options()("generator", "Generator name", cxxopts::value<std::string>())("h,help", "Show help");

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

    Json output;
    output["generator"]             = meta->name;
    output["supports_unique"]       = meta->supports_unique;
    output["supports_data_linkage"] = meta->supports_data_linkage;
    output["linkage_module"]        = meta->linkage_module;

    Json fields = Json::array();
    Json params = Json::array();
    for (const auto& param : meta->config_params) {
        fields.push_back(param.name);
        params.push_back(Json{{"name", param.name}, {"description", param.description}, {"required", param.required}});
    }
    output["config_fields"]   = fields;
    output["config_params"]   = params;
    output["config_template"] = meta->config_template;

    std::cout << output.dump(2) << "\n";
    return kExitOk;
}

}  // namespace data_generator::cli
