// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file command_generate.cpp

#include "cli/command_generate.h"

#include <cxxopts.hpp>
#include <iostream>
#include <optional>
#include <string>

#include "cli/cli_shared.h"
#include "cli/exit_codes.h"
#include "core/configuration.h"
#include "core/executor.h"
#include "core/workspace.h"
#include "database/db_metadata.h"
#include "database/db_url_parser.h"
#include "logging/logger.h"
#include "output/output_backend.h"

namespace data_generator::cli {

namespace {

std::optional<database::DbType> parse_db_type(const std::string& value) {
    if (value == "mysql") { return database::DbType::Mysql; }
    if (value == "postgresql") { return database::DbType::Postgresql; }
    if (value == "sqlite") { return database::DbType::Sqlite; }
    if (value == "oracle") { return database::DbType::Oracle; }
    return std::nullopt;
}

}  // namespace

int CommandGenerate::run(const std::vector<std::string>& args) {
    cxxopts::Options options("data-generator generate", "generate dataset from JSON config.");
    options.add_options()
        ("input", "Input JSON file", cxxopts::value<std::string>())
        ("output", "Output filename under workspace root", cxxopts::value<std::string>())
        ("rows", "Override rows", cxxopts::value<int>())
        ("format", "Override format (csv|json|sql)", cxxopts::value<std::string>())
        ("threads", "Generator threads for eligible workloads", cxxopts::value<std::size_t>()->default_value("1"))
        ("output-dest", "Output destination (file|database)", cxxopts::value<std::string>())
        ("url", "Database URL", cxxopts::value<std::string>())
        ("table", "Target table name", cxxopts::value<std::string>())
        ("db-type", "Database type for URL build (mysql|postgresql|sqlite|oracle)", cxxopts::value<std::string>())
        ("db-user", "Database user", cxxopts::value<std::string>())
        ("db-password", "Database password", cxxopts::value<std::string>())
        ("db-host", "Database host", cxxopts::value<std::string>())
        ("db-port", "Database port", cxxopts::value<int>())
        ("db-name", "Database name", cxxopts::value<std::string>())
        ("insert-mode", "Insert mode (auto|insert|bulk|load)", cxxopts::value<std::string>())
        ("batch-size", "Batch size", cxxopts::value<int>())
        ("queue-size", "Queue size", cxxopts::value<int>())
        ("db-threads", "Database worker threads", cxxopts::value<int>())
        ("transaction-mode", "Transaction mode (per-batch|per-run|none)", cxxopts::value<std::string>())
        ("error-policy", "error policy (stop|continue|rollback-batch|rollback-all)", cxxopts::value<std::string>())
        ("rate-limit", "Rate limit rows/s", cxxopts::value<int>())
        ("workspace", "Workspace root path", cxxopts::value<std::string>())
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

        if (result.count("format")) {
            const auto parsed = core::parse_output_format(result["format"].as<std::string>());
            if (!parsed.has_value()) {
                std::cerr << "--format must be one of: csv, json, sql\n";
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

        if (result.count("output")) {
            const std::string output_name = result["output"].as<std::string>();
            if (!core::is_workspace_local_filename(output_name)) {
                std::cerr << "--output must be a filename only (no directory), file is created in workspace root\n";
                return exit_codes::kUsage;
            }
            overrides.output_path = output_name;
        }

        if (result.count("output-dest")) {
            const auto parsed = core::parse_output_destination(result["output-dest"].as<std::string>());
            if (!parsed.has_value()) {
                std::cerr << "--output-dest must be one of: file, database\n";
                return exit_codes::kUsage;
            }
            overrides.destination = parsed;
        }

        if (result.count("url")) { overrides.database_url = result["url"].as<std::string>(); }

        if (result.count("insert-mode")) {
            const auto parsed = core::parse_insert_mode(result["insert-mode"].as<std::string>());
            if (!parsed.has_value()) {
                std::cerr << "--insert-mode must be one of: auto, insert, bulk, load\n";
                return exit_codes::kUsage;
            }
            overrides.insert_mode = parsed;
        }

        if (result.count("batch-size")) {
            const int value = result["batch-size"].as<int>();
            if (value <= 0) {
                std::cerr << "--batch-size must be >= 1\n";
                return exit_codes::kUsage;
            }
            overrides.batch_size = value;
        }

        if (result.count("queue-size")) {
            const int value = result["queue-size"].as<int>();
            if (value <= 0) {
                std::cerr << "--queue-size must be >= 1\n";
                return exit_codes::kUsage;
            }
            overrides.queue_size = value;
        }

        if (result.count("db-threads")) {
            const int value = result["db-threads"].as<int>();
            if (value <= 0) {
                std::cerr << "--db-threads must be >= 1\n";
                return exit_codes::kUsage;
            }
            overrides.db_threads = value;
        }

        if (result.count("transaction-mode")) {
            const auto parsed = core::parse_transaction_mode(result["transaction-mode"].as<std::string>());
            if (!parsed.has_value()) {
                std::cerr << "--transaction-mode must be one of: per-batch, per-run, none\n";
                return exit_codes::kUsage;
            }
            overrides.transaction_mode = parsed;
        }

        if (result.count("error-policy")) {
            const auto parsed = core::parse_error_policy(result["error-policy"].as<std::string>());
            if (!parsed.has_value()) {
                std::cerr << "--error-policy must be one of: stop, continue, rollback-batch, rollback-all\n";
                return exit_codes::kUsage;
            }
            overrides.error_policy = parsed;
        }

        if (result.count("rate-limit")) {
            const int value = result["rate-limit"].as<int>();
            if (value <= 0) {
                std::cerr << "--rate-limit must be >= 1\n";
                return exit_codes::kUsage;
            }
            overrides.rate_limit_rows_per_sec = value;
        }

        if (result.count("workspace")) {
            overrides.workspace = result["workspace"].as<std::string>();
            if (overrides.workspace->empty()) {
                std::cerr << "--workspace must not be empty\n";
                return exit_codes::kUsage;
            }
        }

        const bool has_db_parts = result.count("db-type") || result.count("db-user") || result.count("db-password") ||
                                  result.count("db-host") || result.count("db-port") || result.count("db-name");
        if (has_db_parts) {
            if (!result.count("db-type") || !result.count("db-user") || !result.count("db-password") ||
                !result.count("db-host") || !result.count("db-name")) {
                std::cerr << "building URL from CLI requires --db-type --db-user --db-password --db-host --db-name\n";
                return exit_codes::kUsage;
            }

            const auto db_type = parse_db_type(result["db-type"].as<std::string>());
            if (!db_type.has_value()) {
                std::cerr << "--db-type must be one of: mysql, postgresql, sqlite, oracle\n";
                return exit_codes::kUsage;
            }

            const std::optional<int> port = result.count("db-port") ? std::optional<int>(result["db-port"].as<int>())
                                                                      : std::nullopt;
            overrides.database_url = database::build_db_url(
                *db_type,
                result["db-user"].as<std::string>(),
                result["db-password"].as<std::string>(),
                result["db-host"].as<std::string>(),
                port,
                result["db-name"].as<std::string>()
            );
        }

        core::apply_cli_overrides(&cfg, overrides);

        if (cfg.output.destination == core::OutputDestination::Database && cfg.output.database.url.empty()) {
            std::cerr << "database output requires URL (CLI --url or JSON url)\n";
            return exit_codes::kUsage;
        }

        if (cfg.output.destination == core::OutputDestination::Database && result.count("format")) {
            std::cerr << "--format is ignored when --output-dest database\n";
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

        logger.info(
            "startup rows=" + std::to_string(cfg.rows) +
            " version=1.0 workspace=" + cfg.workspace +
            " destination=" + core::output_destination_to_string(cfg.output.destination) +
            " format=" + core::output_format_to_string(cfg.format) +
            " threads=" + std::to_string(exec_opts.requested_threads)
        );

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
