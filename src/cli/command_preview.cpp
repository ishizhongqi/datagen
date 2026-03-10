// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file command_preview.cpp

#include "cli/command_preview.h"

#include <cxxopts.hpp>
#include <iostream>
#include <optional>
#include <string>

#include "cli/cli_shared.h"
#include "cli/exit_codes.h"
#include "core/configuration.h"
#include "core/executor.h"
#include "core/serialization.h"

namespace data_generator::cli {

int CommandPreview::run(const std::vector<std::string>& args) {
    cxxopts::Options options("data-generator preview", "Generate a single row preview.");
    options.add_options()
        ("input", "Input JSON file", cxxopts::value<std::string>())
        ("field", "Field name or 'all'", cxxopts::value<std::string>()->default_value("all"))
        ("file-format", "Output format (csv|json|sql)", cxxopts::value<std::string>())
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
        const Json root = load_json_from_file(result["input"].as<std::string>());

        core::GenerationConfig             cfg;
        std::vector<core::ValidationIssue> issues;
        if (!core::parse_generation_config(root, core::ParseMode::AllowMissingOutputSettings, &cfg, &issues)) {
            print_validation_issues(issues, std::cerr);
            return exit_codes::kRuntimeFailure;
        }

        if (result.count("file-format")) {
            const auto parsed = core::parse_output_format(result["file-format"].as<std::string>());
            if (!parsed.has_value()) {
                std::cerr << "Unsupported format. Use csv|json|sql\n";
                return exit_codes::kUsage;
            }
            cfg.format = *parsed;
        }

        const auto        row          = core::preview_row(cfg);
        const std::string field_filter = result["field"].as<std::string>();

        if (!field_filter.empty() && field_filter != "all") {
            for (size_t i = 0; i < cfg.fields.size(); ++i) {
                if (cfg.fields[i].name == field_filter) {
                    if (!row[i].has_value()) {
                        std::cout << "NULL\n";
                    } else {
                        std::cout << *row[i] << "\n";
                    }
                    return exit_codes::kOk;
                }
            }
            std::cerr << "Field not found: " << field_filter << "\n";
            return exit_codes::kRuntimeFailure;
        }

        std::vector<std::string> columns;
        columns.reserve(cfg.fields.size());
        for (const auto& field : cfg.fields) { columns.push_back(field.name); }

        std::vector<core::Row> rows = {row};
        switch (cfg.format) {
        case core::OutputFormat::Csv:
            for (size_t i = 0; i < row.size(); ++i) {
                if (i > 0) { std::cout << ","; }
                std::cout << core::csv_escape(row[i].value_or(""));
            }
            std::cout << "\n";
            break;
        case core::OutputFormat::Json: core::write_json(columns, rows, std::cout); break;
        case core::OutputFormat::Sql : core::write_sql(columns, rows, "preview_table", std::cout); break;
        }

        return exit_codes::kOk;
    } catch (const std::exception& ex) {
        std::cerr << "Preview failed: " << ex.what() << "\n";
        return exit_codes::kRuntimeFailure;
    }
}

}  // namespace data_generator::cli
