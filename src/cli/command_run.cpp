// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file command_run.cpp

#include "cli/command_run.h"

#include <cxxopts.hpp>
#include <iostream>
#include <string>

#include "cli/cli_shared.h"
#include "cli/exit_codes.h"
#include "config/configuration.h"
#include "engine/executor.h"
#include "logging/logger.h"
#include "output/output_backend.h"
#include "utils/workspace.h"

namespace data_generator::cli {

namespace {

std::string build_startup_log_line(const config::GenerationConfig& cfg) {
    std::string message =
        "startup rows=" + std::to_string(cfg.rows) +
        " version=" + version_string() +
        " destination=" + config::output_type_to_string(cfg.output.type);
    if (cfg.output.type == config::OutputType::File) {
        message += " format=" + config::output_format_to_string(cfg.output.file.format);
    }
    message += " threads=1";
    return message;
}

}  // namespace

int CommandRun::run(const std::vector<std::string>& args) {
    cxxopts::Options options("data-generator run", "Generate dataset from JSON config.");
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
        if (result.count("output")) {
            overrides.output_path = result["output"].as<std::string>();
        }

        config::apply_cli_overrides(&cfg, overrides);

        if (cfg.output.type == config::OutputType::Database && overrides.output_path.has_value()) {
            std::cerr << "--output is ignored when output.type=database\n";
        }

        std::string workspace_error;
        const utils::WorkspaceLayout layout = utils::ensure_workspace_layout(cfg.workspace, &workspace_error);
        if (layout.root.empty()) {
            std::cerr << "Workspace initialization failed: " << workspace_error << "\n";
            return exit_codes::kRuntimeFailure;
        }
        cfg.workspace = layout.root.string();

        logging::Logger& logger = logging::Logger::instance();
        logger.set_echo_to_stderr(true);
        logger.configure_generate_logs(layout.generate_log_file.string(), layout.error_log_file.string());

        engine::ExecutionOptions exec_opts;
        exec_opts.requested_threads = 1;

        logger.info(build_startup_log_line(cfg));

        auto backend = output::make_output_backend(cfg);
        const output::OutputStats stats = backend->generate(cfg, exec_opts);

        if (stats.execution_info.fallback_to_single_thread) {
            logger.warn("parallel generation fallback to single thread: " + stats.execution_info.fallback_reason);
        }

        logger.info(
            "run completed rows_generated=" + std::to_string(stats.rows_generated) +
            " rows_imported=" + std::to_string(stats.rows_written)
        );
        logger.disable_file_logging();
        return exit_codes::kOk;
    } catch (const std::exception& ex) {
        logging::Logger::instance().error(std::string("run failed: ") + ex.what());
        logging::Logger::instance().disable_file_logging();
        std::cerr << "run failed: " << ex.what() << "\n";
        return exit_codes::kRuntimeFailure;
    }
}

}  // namespace data_generator::cli
