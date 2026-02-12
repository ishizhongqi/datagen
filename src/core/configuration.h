// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file configuration.h

#ifndef DATA_GENERATOR_CORE_CONFIGURATION_H
#define DATA_GENERATOR_CORE_CONFIGURATION_H

#include <nlohmann/json.hpp>
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

struct ValidationIssue {
    std::string path;
    std::string message;
};

enum class ParseMode {
    RequireOutputSettings,
    AllowMissingOutputSettings,
};

struct NullPolicy {
    bool                       configured    = false;
    bool                       null_if_empty = false;
    std::optional<std::string> null_literal;
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
    bool                   include_create_table = true;
    NullPolicy             null_policy;
    std::vector<FieldSpec> fields;
    Json                   root;
};

std::optional<OutputFormat> parse_output_format(const std::string& value);
std::string                 output_format_to_string(OutputFormat format);

bool parse_generation_config(
    const Json&                   root,
    ParseMode                     mode,
    GenerationConfig*             out,
    std::vector<ValidationIssue>* issues
);

void apply_cli_overrides(
    GenerationConfig*                  cfg,
    const std::optional<int>&          rows,
    const std::optional<OutputFormat>& format,
    const std::optional<std::string>&  table_name,
    const std::optional<bool>&         include_create_table
);

}  // namespace data_generator::core

#endif
