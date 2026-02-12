// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file command_validate.cpp

#include "cli/command_validate.h"

#include <cxxopts.hpp>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

#include "cli/cli_shared.h"
#include "cli/exit_codes.h"
#include "core/configuration.h"
#include "core/executor.h"

namespace data_generator::cli {

int CommandValidate::run(const std::vector<std::string>& args) {
    cxxopts::Options options("data-generator validate", "Validate JSON configuration.");
    options.add_options()
        ("input", "Input JSON file", cxxopts::value<std::string>())
        ("require-output", "Require rows and output_format")
        ("h,help", "Show help");

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

    if (!result.count("input")) {
        std::cerr << "Missing required --input\n";
        std::cerr << options.help() << "\n";
        return exit_codes::kUsage;
    }

    try {
        const Json                         root = load_json_from_file(result["input"].as<std::string>());
        core::GenerationConfig             cfg;
        std::vector<core::ValidationIssue> issues;
        const auto mode = result.count("require-output") ? core::ParseMode::RequireOutputSettings
                                                         : core::ParseMode::AllowMissingOutputSettings;

        if (!core::parse_generation_config(root, mode, &cfg, &issues)) {
            print_validation_issues(issues, std::cerr);
            return exit_codes::kRuntimeFailure;
        }

        try {
            (void)core::preview_row(cfg, std::nullopt);
        } catch (const std::exception& ex) {
            std::cerr << "Validation error: runtime generator construction failed: " << ex.what() << "\n";
            return exit_codes::kRuntimeFailure;
        }

        std::cout << "Validation success. fields=" << cfg.fields.size() << " rows=" << cfg.rows
                  << " format=" << core::output_format_to_string(cfg.format) << "\n";
        return exit_codes::kOk;
    } catch (const std::exception& ex) {
        std::cerr << "Validation failed: " << ex.what() << "\n";
        return exit_codes::kRuntimeFailure;
    }
}

}  // namespace data_generator::cli
