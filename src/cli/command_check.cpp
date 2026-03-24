// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file command_check.cpp

#include "cli/command_check.h"

#include <cxxopts.hpp>
#include <iostream>
#include <string>
#include <vector>

#include "cli/cli_shared.h"
#include "cli/exit_codes.h"
#include "config/configuration.h"
#include "engine/executor.h"
#include "output/database/db_url_parser.h"
#include "output/database/drivers/idatabase_driver.h"

namespace data_generator::cli {

namespace {

bool validate_database_connection(const config::GenerationConfig& cfg, std::string& error_message) {
    database::DbUrl parsed_url;
    if (!database::parse_db_connection(cfg.output.database.connection, &parsed_url, &error_message)) { return false; }

    const auto driver = database::make_database_driver(parsed_url.type);
    if (!driver) {
        error_message = "unsupported database type";
        return false;
    }
    if (!driver->connect(parsed_url, &error_message)) { return false; }

    const bool ok = driver->test_connection(&error_message);
    driver->disconnect();
    return ok;
}

}  // namespace

int CommandCheck::run(const std::vector<std::string>& args) {
    cxxopts::Options options("data-generator check", "Validate JSON configuration.");
    options.add_options()
        ("config", "Input JSON file", cxxopts::value<std::string>())
        ("h,help", "Show help");
    options.parse_positional({"config"});

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

    if (!result.count("config")) {
        std::cerr << "Missing required <json>\n";
        std::cerr << options.help() << "\n";
        return exit_codes::kUsage;
    }

    try {
        const Json                         root = load_json_from_file(result["config"].as<std::string>());
        config::GenerationConfig             cfg;
        std::vector<config::ValidationIssue> issues;

        if (!config::parse_generation_config(root, config::ParseMode::AllowMissingOutputSettings, &cfg, &issues)) {
            print_validation_issues(issues, std::cerr);
            return exit_codes::kRuntimeFailure;
        }
        print_validation_issues(issues, std::cerr);

        std::vector<config::ValidationIssue> warnings;
        if (cfg.output.type == config::OutputType::Database) {
            if (cfg.output.database.connection.empty()) {
                warnings.push_back(config::ValidationIssue{
                    .warning = true,
                    .path = "$.output.database.connection",
                    .message = "database output requires connection in odbc://... or sqlite://... format",
                });
            }
            if (cfg.output.database.table.empty()) {
                warnings.push_back(config::ValidationIssue{
                    .warning = true,
                    .path = "$.output.database.table",
                    .message = "database output requires target table name",
                });
            }
        }

        print_validation_issues(warnings, std::cerr);

        try {
            (void)engine::preview_row(cfg);
        } catch (const std::exception& ex) {
            std::cerr << "Validation error: runtime generator construction failed: " << ex.what() << "\n";
            return exit_codes::kRuntimeFailure;
        }

        if (cfg.output.type == config::OutputType::Database && !cfg.output.database.connection.empty()) {
            std::string db_error;
            if (!validate_database_connection(cfg, db_error)) {
                std::cerr << "Validation error: database connection check failed: " << db_error << "\n";
                return exit_codes::kRuntimeFailure;
            }
        }

        std::cout << "Validation success. fields=" << cfg.fields.size() << " rows=" << cfg.rows;
        if (cfg.output.type == config::OutputType::File) {
            std::cout << " format=" << config::output_format_to_string(cfg.output.file.format);
        }
        std::cout << " warnings=" << (issues.size() + warnings.size()) << "\n";
        return exit_codes::kOk;
    } catch (const std::exception& ex) {
        std::cerr << "Validation failed: " << ex.what() << "\n";
        return exit_codes::kRuntimeFailure;
    }
}

}  // namespace data_generator::cli
