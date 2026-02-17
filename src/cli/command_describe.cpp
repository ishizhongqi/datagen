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
#include "cli/exit_codes.h"
#include "cli/generator_catalog.h"

namespace data_generator::cli {

int CommandDescribe::run(const std::vector<std::string>& args) {
    cxxopts::Options options("data-generator describe", "Describe generator metadata.");
    options.add_options()("generator", "Generator name", cxxopts::value<std::string>())("json", "Output JSON")(
        "h,help",
        "Show help"
    );

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

    if (!result.count("generator")) {
        std::cerr << "Missing required --generator\n";
        std::cerr << options.help() << "\n";
        return exit_codes::kUsage;
    }

    const std::string        generator_name = result["generator"].as<std::string>();
    const GeneratorMetadata* meta           = find_generator_metadata(generator_name);
    if (!meta) {
        std::cerr << "Unknown generator: " << generator_name << "\n";
        return exit_codes::kUsage;
    }

    if (!result.count("json")) {
        const auto lines = build_describe_text_lines(*meta);
        for (const auto& line : lines) { std::cout << line << "\n"; }
        return exit_codes::kOk;
    }

    OrderedJson output = OrderedJson::object();
    output["generator"] = meta->name;
    output["module"]    = meta->module.empty() ? OrderedJson(nullptr) : OrderedJson(meta->module);
    output["supports"]  = OrderedJson{{"unique", meta->supports_unique}, {"data_linkage", meta->supports_data_linkage}};

    OrderedJson fields = OrderedJson::array();
    for (const auto& param : meta->config_params) {
        OrderedJson default_value = nullptr;
        if (meta->config_template.contains(param.name)) {
            default_value = to_ordered_json(meta->config_template.at(param.name));
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

    output["config"] = OrderedJson{{"fields", fields}, {"template", build_ordered_config_template(*meta)}};

    std::cout << output.dump(2) << "\n";
    return exit_codes::kOk;
}

}  // namespace data_generator::cli
