// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file command_validate.cpp

#include "cli/command_validate.h"

#include <cxxopts.hpp>
#include <iostream>
#include <string>
#include <vector>

#include "cli/cli_shared.h"

namespace data_generator::cli {

namespace {

constexpr int kExitOk      = 0;
constexpr int kExitUsage   = 2;
constexpr int kExitInvalid = 3;

}  // namespace

int CommandValidate::run(const std::vector<std::string>& args) {
    cxxopts::Options options("data-generator validate", "Validate JSON configuration.");
    options.add_options()("input", "Input JSON file", cxxopts::value<std::string>())("h,help", "Show help");

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

    if (!result.count("input")) {
        std::cerr << "Missing required --input\n";
        std::cerr << options.help() << "\n";
        return kExitUsage;
    }

    std::vector<std::string> errors;
    Json                     root;
    try {
        root = load_json_from_file(result["input"].as<std::string>());
    } catch (const std::exception& ex) {
        std::cerr << "Failed to load json: " << ex.what() << "\n";
        return kExitInvalid;
    }

    if (!validate_config(root, false, &errors)) {
        print_validation_errors(errors, std::cerr);
        return kExitInvalid;
    }

    std::cout << "Validation success.\n";
    return kExitOk;
}

}  // namespace data_generator::cli
