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
#include "database/db_url_parser.h"
#include "database/driver/idatabase_driver.h"

namespace data_generator::cli {

namespace {

bool validate_database_connection(const core::GenerationConfig& cfg, std::string* error_message) {
    database::DbUrl parsed_url;
    if (!database::parse_db_url(cfg.output.database.url, &parsed_url, error_message)) { return false; }

    const auto driver = database::make_database_driver(parsed_url.type);
    if (!driver) {
        if (error_message) { *error_message = "unsupported database type"; }
        return false;
    }
    if (!driver->connect(parsed_url, error_message)) { return false; }

    const bool ok = driver->test_connection(error_message);
    driver->disconnect();
    return ok;
}

}  // namespace

int CommandValidate::run(const std::vector<std::string>& args) {
    cxxopts::Options options("data-generator validate", "Validate JSON configuration.");
    options.add_options()
        ("input", "Input JSON file", cxxopts::value<std::string>())
        ("db-only", "Validate database connection only")
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

        if (!core::parse_generation_config(root, core::ParseMode::AllowMissingOutputSettings, &cfg, &issues)) {
            print_validation_issues(issues, std::cerr);
            return exit_codes::kRuntimeFailure;
        }
        print_validation_issues(issues, std::cerr);

        if (result.count("db-only")) {
            if (cfg.output.database.url.empty()) {
                std::cerr << "Validation failed: database URL/ODBC connection string is required for --db-only\n";
                return exit_codes::kRuntimeFailure;
            }

            std::string db_error;
            if (!validate_database_connection(cfg, &db_error)) {
                std::cerr << "Validation failed: database connection check failed: " << db_error << "\n";
                return exit_codes::kRuntimeFailure;
            }

            std::cout << "Validation success. database connection ok\n";
            return exit_codes::kOk;
        }

        std::vector<core::ValidationIssue> warnings;
        if (cfg.output.destination == core::OutputDestination::Database) {
            if (cfg.output.database.url.empty()) {
                warnings.push_back(core::ValidationIssue{
                    .warning = true,
                    .path = "$.url",
                    .message = "database generate requires URL/ODBC connection string (CLI can override)",
                });
            }
            if (cfg.table_name.empty()) {
                warnings.push_back(core::ValidationIssue{
                    .warning = true,
                    .path = "$.table",
                    .message = "database generate requires target table name",
                });
            }
        }

        print_validation_issues(warnings, std::cerr);

        try {
            (void)core::preview_row(cfg);
        } catch (const std::exception& ex) {
            std::cerr << "Validation error: runtime generator construction failed: " << ex.what() << "\n";
            return exit_codes::kRuntimeFailure;
        }

        if (cfg.output.destination == core::OutputDestination::Database && !cfg.output.database.url.empty()) {
            std::string db_error;
            if (!validate_database_connection(cfg, &db_error)) {
                std::cerr << "Validation error: database connection check failed: " << db_error << "\n";
                return exit_codes::kRuntimeFailure;
            }
        }

        std::cout << "Validation success. fields=" << cfg.fields.size() << " rows=" << cfg.rows
                  << " format=" << core::output_format_to_string(cfg.format)
                  << " warnings=" << (issues.size() + warnings.size()) << "\n";
        return exit_codes::kOk;
    } catch (const std::exception& ex) {
        std::cerr << "Validation failed: " << ex.what() << "\n";
        return exit_codes::kRuntimeFailure;
    }
}

}  // namespace data_generator::cli
