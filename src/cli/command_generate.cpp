// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file command_generate.cpp

#include "cli/command_generate.h"

#include <cxxopts.hpp>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>

#include "cli/cli_shared.h"
#include "cli/exit_codes.h"
#include "core/configuration.h"
#include "core/executor.h"

namespace data_generator::cli {

int CommandGenerate::run(const std::vector<std::string>& args) {
    cxxopts::Options options("data-generator generate", "Generate dataset from JSON config.");
    options.add_options()
        ("input", "Input JSON file", cxxopts::value<std::string>())
        ("output", "Output file path", cxxopts::value<std::string>())
        ("rows", "Override rows", cxxopts::value<int>())
        ("format", "Override format (csv|json|sql)", cxxopts::value<std::string>())
        ("threads", "Worker threads for eligible workloads", cxxopts::value<std::size_t>()->default_value("1"))
        ("table", "Override SQL table name", cxxopts::value<std::string>())
        ("create-table", "Force CREATE TABLE for SQL")
        ("no-create-table", "Disable CREATE TABLE for SQL")
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
        const std::string input_path = result["input"].as<std::string>();
        const Json        root       = load_json_from_file(input_path);

        core::GenerationConfig             cfg;
        std::vector<core::ValidationIssue> issues;
        if (!core::parse_generation_config(root, core::ParseMode::RequireOutputSettings, &cfg, &issues)) {
            print_validation_issues(issues, std::cerr);
            return exit_codes::kRuntimeFailure;
        }

        std::optional<int> rows_override;
        if (result.count("rows")) {
            const int rows = result["rows"].as<int>();
            if (rows <= 0) {
                std::cerr << "--rows must be a positive integer\n";
                return exit_codes::kUsage;
            }
            rows_override = rows;
        }

        std::optional<core::OutputFormat> format_override;
        if (result.count("format")) {
            const auto parsed = core::parse_output_format(result["format"].as<std::string>());
            if (!parsed.has_value()) {
                std::cerr << "--format must be one of: csv, json, sql\n";
                return exit_codes::kUsage;
            }
            format_override = parsed;
        }

        std::optional<std::string> table_override;
        if (result.count("table")) {
            table_override = result["table"].as<std::string>();
            if (table_override->empty()) {
                std::cerr << "--table must not be empty\n";
                return exit_codes::kUsage;
            }
        }

        std::optional<bool> create_table_override;
        if (result.count("create-table") && result.count("no-create-table")) {
            std::cerr << "Use either --create-table or --no-create-table, not both\n";
            return exit_codes::kUsage;
        }
        if (result.count("create-table")) { create_table_override = true; }
        if (result.count("no-create-table")) { create_table_override = false; }

        core::apply_cli_overrides(&cfg, rows_override, format_override, table_override, create_table_override);

        std::ostream* output = &std::cout;
        std::ofstream file_output;
        if (result.count("output")) {
            const std::string path = result["output"].as<std::string>();
            file_output.open(path, std::ios::trunc);
            if (!file_output) {
                std::cerr << "Failed to open output file: " << path << "\n";
                return exit_codes::kRuntimeFailure;
            }
            output = &file_output;
        }

        core::ExecutionOptions exec_opts;
        exec_opts.requested_threads = result["threads"].as<std::size_t>();
        if (exec_opts.requested_threads == 0) {
            std::cerr << "--threads must be >= 1\n";
            return exit_codes::kUsage;
        }

        const auto generation_result = core::generate_to_stream(cfg, exec_opts, *output);
        if (generation_result.info.fallback_to_single_thread) {
            std::cerr << "Info: parallel generation fallback to single thread: "
                      << generation_result.info.fallback_reason << "\n";
        }

        return exit_codes::kOk;
    } catch (const std::exception& ex) {
        std::cerr << "Generate failed: " << ex.what() << "\n";
        return exit_codes::kRuntimeFailure;
    }
}

}  // namespace data_generator::cli
