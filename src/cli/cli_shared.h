// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file cli_shared.h

#ifndef DATA_GENERATOR_CLI_SHARED_H
#define DATA_GENERATOR_CLI_SHARED_H

#include <cxxopts.hpp>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "generators/generator_base.h"
#include "generators/generator_registry.h"

namespace data_generator::cli {

using Json = nlohmann::json;

struct FieldGenerator {
    std::string                            name;
    std::unique_ptr<generator::IGenerator> generator;
};

generator::GeneratorRegistry build_registry();

Json load_json_from_file(const std::string& path);

int resolve_row_count(const Json& root, int cli_rows, int default_rows);

std::vector<FieldGenerator> build_field_generators(const Json& root, int rows);

std::string csv_escape(const std::string& value);

std::string sql_escape(const std::string& value);

cxxopts::ParseResult parse_options(cxxopts::Options& options, const std::vector<std::string>& args);

}  // namespace data_generator::cli

#endif  // DATA_GENERATOR_CLI_SHARED_H
