// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file command_generate.cpp

#include "cli/command_generate.h"

#include <cxxopts.hpp>
#include <iostream>
#include <string>

#include "cli/cli_shared.h"
#include "cli/exit_codes.h"
#include "core/configuration.h"
#include "core/executor.h"
#include "core/workspace.h"
#include "logging/logger.h"
#include "output/output_backend.h"

namespace data_generator::cli {

namespace {

std::string build_startup_log_line(const core::GenerationConfig& cfg, const std::size_t requested_threads) {
    std::string message =
        "startup rows=" + std::to_string(cfg.rows) +
        " version=1.0 destination=" + core::output_destination_to_string(cfg.output.destination);
    if (cfg.output.destination == core::OutputDestination::File) {
        message += " format=" + core::output_format_to_string(cfg.format);
    }
    message += " threads=" + std::to_string(requested_threads);
    return message;
}

}  // namespace

int CommandGenerate::run(const std::vector<std::string>& args) {
    cxxopts::Options options("data-generator generate", "generate dataset from JSON config.");
    options.add_options()
        ("input", "Input JSON file", cxxopts::value<std::string>())
        (
            "file-output",
            "Output file path for file destination. If omitted, writes dgresult_YYYYmmddHHMMSS.<format> in current directory",
            cxxopts::value<std::string>()
        )
        ("output", "Output file path for file destination (alias of --file-output)", cxxopts::value<std::string>())
        ("rows", "Override rows", cxxopts::value<int>())
        ("destination", "Output destination (file|database)", cxxopts::value<std::string>())
        ("file-format", "Override file format (csv|json|sql)", cxxopts::value<std::string>())
        ("database-url", "Database URL or ODBC connection string", cxxopts::value<std::string>())
        ("table", "Target table name", cxxopts::value<std::string>())
        ("threads", "Generator threads for eligible workloads", cxxopts::value<std::size_t>()->default_value("1"))
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

        core::CliOverrides overrides;

        if (result.count("rows")) {
            const int rows = result["rows"].as<int>();
            if (rows <= 0) {
                std::cerr << "--rows must be a positive integer\n";
                return exit_codes::kUsage;
            }
            overrides.rows = rows;
        }

        if (result.count("file-format")) {
            const auto parsed = core::parse_output_format(result["file-format"].as<std::string>());
            if (!parsed.has_value()) {
                std::cerr << "--file-format must be one of: csv, json, sql\n";
                return exit_codes::kUsage;
            }
            overrides.format = parsed;
        }

        if (result.count("table")) {
            overrides.table_name = result["table"].as<std::string>();
            if (overrides.table_name->empty()) {
                std::cerr << "--table must not be empty\n";
                return exit_codes::kUsage;
            }
        }

        if (result.count("file-output")) {
            overrides.output_path = result["file-output"].as<std::string>();
        } else if (result.count("output")) {
            overrides.output_path = result["output"].as<std::string>();
        }

        if (result.count("destination")) {
            const auto parsed = core::parse_output_destination(result["destination"].as<std::string>());
            if (!parsed.has_value()) {
                std::cerr << "--destination must be one of: file, database\n";
                return exit_codes::kUsage;
            }
            overrides.destination = parsed;
        }

        if (result.count("database-url")) { overrides.database_url = result["database-url"].as<std::string>(); }

        core::apply_cli_overrides(&cfg, overrides);

        if (cfg.output.destination == core::OutputDestination::Database && cfg.output.database.url.empty()) {
            std::cerr << "database output requires URL/ODBC connection string (CLI --database-url or JSON)\n";
            return exit_codes::kUsage;
        }

        if (cfg.output.destination == core::OutputDestination::Database && result.count("file-format")) {
            std::cerr << "--file-format is ignored when --destination database\n";
        }

        std::string workspace_error;
        const core::WorkspaceLayout layout = core::ensure_workspace_layout(cfg.workspace, &workspace_error);
        if (layout.root.empty()) {
            std::cerr << "Workspace initialization failed: " << workspace_error << "\n";
            return exit_codes::kRuntimeFailure;
        }
        cfg.workspace = layout.root.string();

        logging::Logger& logger = logging::Logger::instance();
        logger.set_echo_to_stderr(true);
        logger.configure_generate_logs(layout.generate_log_file.string(), layout.error_log_file.string());

        core::ExecutionOptions exec_opts;
        exec_opts.requested_threads = result["threads"].as<std::size_t>();
        if (exec_opts.requested_threads == 0) {
            std::cerr << "--threads must be >= 1\n";
            return exit_codes::kUsage;
        }

        logger.info(build_startup_log_line(cfg, exec_opts.requested_threads));

        auto backend = output::make_output_backend(cfg);
        const output::OutputStats stats = backend->generate(cfg, exec_opts);

        if (stats.execution_info.fallback_to_single_thread) {
            logger.warn("parallel generation fallback to single thread: " + stats.execution_info.fallback_reason);
        }

        logger.info(
            "generate completed rows_generated=" + std::to_string(stats.rows_generated) +
            " rows_imported=" + std::to_string(stats.rows_written)
        );
        logger.disable_file_logging();
        return exit_codes::kOk;
    } catch (const std::exception& ex) {
        logging::Logger::instance().error(std::string("generate failed: ") + ex.what());
        logging::Logger::instance().disable_file_logging();
        std::cerr << "generate failed: " << ex.what() << "\n";
        return exit_codes::kRuntimeFailure;
    }
}

}  // namespace data_generator::cli
