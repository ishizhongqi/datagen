// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file command_run.cpp

#include "cli/command_run.h"

#include <cxxopts.hpp>
#include <iostream>
#include <sstream>
#include <string>

#include "cli/cli_shared.h"
#include "cli/exit_codes.h"
#include "config/configuration.h"
#include "engine/executor.h"
#include "logging/logger.h"
#include "output/output_backend.h"
#include "utils/workspace.h"

namespace datagen::cli {

namespace {

using Json = config::Json;

std::string bool_to_text(const bool value) {
    return value ? "true" : "false";
}

std::string escape_log_text(const std::string& text) {
    std::string escaped;
    escaped.reserve(text.size());
    for (const char ch : text) {
        switch (ch) {
        case '\\': escaped += "\\\\"; break;
        case '\n': escaped += "\\n"; break;
        case '\r': escaped += "\\r"; break;
        case '\t': escaped += "\\t"; break;
        default  : escaped.push_back(ch); break;
        }
    }
    return escaped;
}

std::string json_value_to_log_text(const Json& value) {
    if (value.is_string()) { return escape_log_text(value.get<std::string>()); }
    if (value.is_boolean()) { return bool_to_text(value.get<bool>()); }
    if (value.is_number_integer() || value.is_number_unsigned() || value.is_number_float()) { return value.dump(); }
    if (value.is_null()) { return "null"; }
    return value.dump();
}

bool is_scalar_json_value(const Json& value) {
    return value.is_string() || value.is_boolean() || value.is_number() || value.is_null();
}

void append_field_log_entries(
    const Json&                 value,
    const std::string&          prefix,
    std::vector<std::string>*   entries
) {
    if (!entries || prefix.empty()) { return; }

    if (value.is_object()) {
        for (auto it = value.begin(); it != value.end(); ++it) {
            const std::string child_prefix = prefix + "." + it.key();
            append_field_log_entries(it.value(), child_prefix, entries);
        }
        return;
    }

    if (value.is_array()) {
        bool all_scalar = true;
        for (const auto& item : value) {
            if (!is_scalar_json_value(item)) {
                all_scalar = false;
                break;
            }
        }

        if (all_scalar) {
            std::ostringstream oss;
            bool               first = true;
            for (const auto& item : value) {
                if (!first) { oss << "|"; }
                first = false;
                oss << json_value_to_log_text(item);
            }
            entries->push_back(prefix + "=" + oss.str());
        } else {
            entries->push_back(prefix + "=" + value.dump());
        }
        return;
    }

    entries->push_back(prefix + "=" + json_value_to_log_text(value));
}

std::string build_field_log_line(const config::FieldSpec& field) {
    std::ostringstream         oss;
    std::vector<std::string>   entries;
    oss << "Field name=" << field.name << ", generator=" << field.generator;

    if (field.raw.is_object()) {
        for (auto it = field.raw.begin(); it != field.raw.end(); ++it) {
            if (it.key() == "name" || it.key() == "generator") { continue; }
            append_field_log_entries(it.value(), it.key(), &entries);
        }
    }

    for (const auto& entry : entries) { oss << ", " << entry; }
    return oss.str();
}

std::string build_program_line() {
    return "Program: " + program_display_name() + ", Version: " + version_string() + ", GitHub: " + github_url();
}

std::string build_file_output_line(const config::GenerationConfig& cfg) {
    std::ostringstream oss;
    oss << "File output: format=" << config::output_format_to_string(cfg.output.file.format);

    switch (cfg.output.file.format) {
    case config::OutputFormat::Csv:
        oss << ", options.header=" << bool_to_text(cfg.output.file.csv.header)
            << ", options.line_ending=" << config::line_ending_to_string(cfg.output.file.csv.line_ending);
        break;
    case config::OutputFormat::TabDelimited:
        oss << ", options.header=" << bool_to_text(cfg.output.file.tab_delimited.header)
            << ", options.line_ending=" << config::line_ending_to_string(cfg.output.file.tab_delimited.line_ending);
        break;
    case config::OutputFormat::JsonFormat:
        oss << ", options.array=" << bool_to_text(cfg.output.file.json.array)
            << ", options.include_null=" << bool_to_text(cfg.output.file.json.include_null);
        break;
    case config::OutputFormat::Sql:
        oss << ", options.table=" << cfg.output.file.sql.table
            << ", options.create_table=" << bool_to_text(cfg.output.file.sql.create_table);
        break;
    case config::OutputFormat::Custom:
        oss << ", options.delimiter=" << cfg.output.file.custom.delimiter
            << ", options.quote=" << cfg.output.file.custom.quote
            << ", options.header=" << bool_to_text(cfg.output.file.custom.header)
            << ", options.line_ending=" << config::line_ending_to_string(cfg.output.file.custom.line_ending);
        break;
    }

    return oss.str();
}

void log_field_lines(const config::GenerationConfig& cfg, logging::Logger* logger) {
    if (!logger) { return; }

    for (const auto& field : cfg.fields) { logger->info(build_field_log_line(field)); }
}

}  // namespace

int CommandRun::run(const std::vector<std::string>& args) {
    cxxopts::Options options(program_display_name() + " run", "Generate dataset from JSON config.");
    options.add_options()
        ("config", "Input JSON file", cxxopts::value<std::string>())
        ("rows", "Override rows", cxxopts::value<int>())
        ("output", "Output file path for file output", cxxopts::value<std::string>())
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
        const std::string input_path = result["config"].as<std::string>();
        const Json        root       = load_json_from_file(input_path);

        config::GenerationConfig             cfg;
        std::vector<config::ValidationIssue> issues;
        if (!config::parse_generation_config(root, config::ParseMode::RequireOutputSettings, &cfg, &issues)) {
            print_validation_issues(issues, std::cerr);
            return exit_codes::kRuntimeFailure;
        }

        config::CliOverrides overrides;
        if (result.count("rows")) {
            const int rows = result["rows"].as<int>();
            if (rows <= 0) {
                std::cerr << "--rows must be a positive integer\n";
                return exit_codes::kUsage;
            }
            overrides.rows = rows;
        }
        if (result.count("output")) { overrides.output_path = result["output"].as<std::string>(); }

        config::apply_cli_overrides(&cfg, overrides);

        if (cfg.output.type == config::OutputType::Database && overrides.output_path.has_value()) {
            std::cerr << "--output is ignored when output.type=database\n";
        }

        std::string                  workspace_error;
        const utils::WorkspaceLayout layout = utils::ensure_workspace_layout(cfg.workspace, &workspace_error);
        if (layout.root.empty()) {
            std::cerr << "Workspace initialization failed: " << workspace_error << "\n";
            return exit_codes::kRuntimeFailure;
        }
        cfg.workspace = layout.root.string();

        logging::Logger& logger = logging::Logger::instance();
        logger.set_echo_to_stderr(true);
        logger.configure_generate_logs(layout.generate_log_file.string(), layout.error_log_file.string());
        logger.info(build_program_line());

        if (cfg.output.type == config::OutputType::File) {
            logger.info("Output type: file");
            logger.info(build_file_output_line(cfg));
            log_field_lines(cfg, &logger);
        }

        engine::ExecutionOptions exec_opts;
        exec_opts.requested_threads = 1;

        auto                      backend = output::make_output_backend(cfg);
        const output::OutputStats stats   = backend->generate(cfg, exec_opts);

        if (cfg.output.type == config::OutputType::File) {
            logger.info(
                "Generation summary: generated=" +
                std::to_string(stats.rows_generated) +
                ", success=" +
                std::to_string(stats.rows_generated) +
                ", failed=0"
            );
            logger.info("Completed");
        }

        logger.disable_file_logging();
        return exit_codes::kOk;
    } catch (const std::exception& ex) {
        logging::Logger& logger = logging::Logger::instance();
        if (!logger.consume_error_emitted()) { logger.error(std::string("Run failed: ") + ex.what()); }
        logger.disable_file_logging();
        return exit_codes::kRuntimeFailure;
    }
}

}  // namespace datagen::cli
