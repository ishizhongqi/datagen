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

namespace data_generator::cli {

int CommandSchema::run(const std::vector<std::string>& args) {
    cxxopts::Options options("data-generator schema", "Generate JSON Schema from generator metadata.");
    options.add_options()("output", "Output schema file path", cxxopts::value<std::string>())("h,help", "Show help");

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

    const std::string schema_text = build_json_schema().dump(2);
    if (!result.count("output")) {
        std::cout << schema_text << "\n";
        return exit_codes::kOk;
    }

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
