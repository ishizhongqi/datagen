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
#include "config/configuration.h"
#include "engine/executor.h"
#include "output/file/csv_writer.h"
#include "output/file/json_writer.h"
#include "output/file/sql_writer.h"

namespace datagen::cli {

int CommandPreview::run(const std::vector<std::string>& args) {
    cxxopts::Options options(program_display_name() + " preview", "Generate preview rows.");
    options.add_options()
        ("config", "Input JSON file", cxxopts::value<std::string>())
        ("rows", "Preview row count", cxxopts::value<int>()->default_value("1"))
        ("field", "Field name", cxxopts::value<std::string>())
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
        const Json root = load_json_from_file(result["config"].as<std::string>());

        config::GenerationConfig             cfg;
        std::vector<config::ValidationIssue> issues;
        if (!config::parse_generation_config(root, config::ParseMode::AllowMissingOutputSettings, &cfg, &issues)) {
            print_validation_issues(issues, std::cerr);
            return exit_codes::kRuntimeFailure;
        }

        const int preview_rows = result["rows"].as<int>();
        if (preview_rows <= 0) {
            std::cerr << "--rows must be a positive integer\n";
            return exit_codes::kUsage;
        }

        const auto        rows         = engine::preview_rows(cfg, static_cast<std::size_t>(preview_rows));
        const std::string field_filter = result.count("field") ? result["field"].as<std::string>() : "";

        if (!field_filter.empty()) {
            for (std::string::size_type i = 0; i < cfg.fields.size(); ++i) {
                if (cfg.fields[i].name == field_filter) {
                    for (const auto& row : rows) {
                        if (!row[i].has_value()) {
                            std::cout << "NULL\n";
                        } else {
                            std::cout << *row[i] << "\n";
                        }
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

        std::vector<bool> boolean_columns;
        boolean_columns.reserve(cfg.fields.size());
        for (const auto& field : cfg.fields) { boolean_columns.push_back(field.generator == "boolean"); }

        const bool                 use_csv_preview = cfg.output.type == config::OutputType::Database;
        const config::OutputFormat format = use_csv_preview ? config::OutputFormat::Csv : cfg.output.file.format;
        output::file::DelimitedWriterOptions delimited_options;
        switch (format) {
        case config::OutputFormat::Csv:
            delimited_options.delimiter   = ",";
            delimited_options.quote       = "\"";
            delimited_options.header      = use_csv_preview ? true : cfg.output.file.csv.header;
            delimited_options.line_ending = cfg.output.file.csv.line_ending;
            output::file::write_delimited(columns, rows, std::cout, delimited_options);
            break;
        case config::OutputFormat::TabDelimited:
            delimited_options.delimiter   = "\t";
            delimited_options.quote       = "\"";
            delimited_options.header      = cfg.output.file.tab_delimited.header;
            delimited_options.line_ending = cfg.output.file.tab_delimited.line_ending;
            output::file::write_delimited(columns, rows, std::cout, delimited_options);
            break;
        case config::OutputFormat::Custom:
            delimited_options.delimiter   = cfg.output.file.custom.delimiter;
            delimited_options.quote       = cfg.output.file.custom.quote;
            delimited_options.header      = cfg.output.file.custom.header;
            delimited_options.line_ending = cfg.output.file.custom.line_ending;
            output::file::write_delimited(columns, rows, std::cout, delimited_options);
            break;
        case config::OutputFormat::JsonFormat:
            output::file::write_json(columns, boolean_columns, rows, std::cout, cfg.output.file.json);
            break;
        case config::OutputFormat::Sql:
            if (cfg.output.file.sql.table.empty()) {
                output::file::write_sql(columns, boolean_columns, rows, "preview_table", false, std::cout);
            } else {
                output::file::write_sql(
                    columns,
                    boolean_columns,
                    rows,
                    cfg.output.file.sql.table,
                    cfg.output.file.sql.create_table,
                    std::cout
                );
            }
            break;
        }

        return exit_codes::kOk;
    } catch (const std::exception& ex) {
        std::cerr << "Preview failed: " << ex.what() << "\n";
        return exit_codes::kRuntimeFailure;
    }
}

}  // namespace datagen::cli
