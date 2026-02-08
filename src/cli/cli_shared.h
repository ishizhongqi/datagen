// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file cli_shared.h

#ifndef DATA_GENERATOR_CLI_SHARED_H
#define DATA_GENERATOR_CLI_SHARED_H

#include <cxxopts.hpp>
#include <memory>
#include <nlohmann/json.hpp>
#include <ostream>
#include <string>
#include <vector>

#include "generators/generator_base.h"
#include "generators/generator_registry.h"
#include "cli/generator_catalog.h"

namespace data_generator::cli {

using Json = nlohmann::json;

struct FieldGenerator {
    std::string                            name;
    std::unique_ptr<generator::IGenerator> generator;
};

/// @brief Build generator registry with all known generators.
generator::GeneratorRegistry build_registry();

/// @brief Load JSON from a file path.
Json load_json_from_file(const std::string& path);

/// @brief Resolve row count based on CLI override, JSON, and default.
int resolve_row_count(const Json& root, int cli_rows, int default_rows);

/// @brief Build generator instances for each field definition.
std::vector<FieldGenerator> build_field_generators(const Json& root, int rows);

/// @brief Escape a string for CSV output.
std::string csv_escape(const std::string& value);

/// @brief Escape a string for SQL output.
std::string sql_escape(const std::string& value);

/// @brief Parse cxxopts options from argv-style arguments.
cxxopts::ParseResult parse_options(cxxopts::Options& options, const std::vector<std::string>& args);

/// @brief Validate JSON configuration and collect errors.
bool validate_config(const Json& root, bool require_output_settings, std::vector<std::string>* errors);

/// @brief Print validation errors in CLI format.
void print_validation_errors(const std::vector<std::string>& errors, std::ostream& output);

/// @brief Build human-readable describe text lines for a generator.
std::vector<std::string> build_describe_text_lines(const GeneratorMetadata& meta);

}  // namespace data_generator::cli

#endif  // DATA_GENERATOR_CLI_SHARED_H
