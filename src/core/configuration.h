// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file configuration.h

#ifndef DATA_GENERATOR_CONFIGURATION_H
#define DATA_GENERATOR_CONFIGURATION_H

#include <nlohmann/json.hpp>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace data_generator::core {

using Json = nlohmann::json;

enum class OutputFormat {
    Csv,
    Json,
    Sql,
};

enum class OutputDestination {
    File,
    Database,
};

enum class InsertMode {
    Auto,
    Insert,
    Bulk,
    Load,
};

enum class TransactionMode {
    PerBatch,
    PerRun,
    None,
};

enum class ErrorPolicy {
    Stop,
    Continue,
    RollbackBatch,
    RollbackAll,
};

struct ValidationIssue {
    bool        warning = false;
    std::string path;
    std::string message;
};

enum class ParseMode {
    RequireOutputSettings,
    AllowMissingOutputSettings,
};

struct NullPolicy {
    bool                       configured = false;
    bool                       null_if_empty = false;
    std::optional<std::string> null_literal;
};

struct FileOutputConfig {
    std::string path;
};

struct DatabaseOutputConfig {
    std::string     url;
    std::string     table_name = "generated_data";
    InsertMode      insert_mode = InsertMode::Auto;
    int             batch_size = 1000;
    int             queue_size = 1024;
    int             db_threads = 2;
    TransactionMode transaction_mode = TransactionMode::PerBatch;
    ErrorPolicy     error_policy = ErrorPolicy::Stop;
    int             rate_limit_rows_per_sec = 20000;
};

struct OutputConfig {
    OutputDestination   destination = OutputDestination::File;
    FileOutputConfig    file;
    DatabaseOutputConfig database;
};

struct FieldSpec {
    std::string                name;
    std::string                generator;
    Json                       raw;
    std::optional<std::string> data_linkage;
    bool                       unique = false;
};

struct GenerationConfig {
    int                    rows                 = 100;
    OutputFormat           format               = OutputFormat::Csv;
    std::string            table_name           = "generated_data";
    std::string            workspace;
    NullPolicy             null_policy;
    std::vector<FieldSpec> fields;
    OutputConfig           output;
    Json                   root;
};

std::optional<OutputFormat> parse_output_format(const std::string& value);
std::string                 output_format_to_string(OutputFormat format);
std::optional<OutputDestination> parse_output_destination(const std::string& value);
std::string                    output_destination_to_string(OutputDestination destination);
std::optional<InsertMode>      parse_insert_mode(const std::string& value);
std::string                    insert_mode_to_string(InsertMode mode);
std::optional<TransactionMode> parse_transaction_mode(const std::string& value);
std::string                    transaction_mode_to_string(TransactionMode mode);
std::optional<ErrorPolicy>     parse_error_policy(const std::string& value);
std::string                    error_policy_to_string(ErrorPolicy policy);

bool parse_generation_config(
    const Json&                   root,
    ParseMode                     mode,
    GenerationConfig*             out,
    std::vector<ValidationIssue>* issues
);

struct CliOverrides {
    std::optional<int>              rows;
    std::optional<OutputFormat>     format;
    std::optional<std::string>      table_name;
    std::optional<OutputDestination> destination;
    std::optional<std::string>      output_path;
    std::optional<std::string>      database_url;
};

void apply_cli_overrides(GenerationConfig* cfg, const CliOverrides& overrides);

}  // namespace data_generator::core

#endif
