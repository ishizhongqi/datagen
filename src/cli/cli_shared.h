// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file cli_shared.h

#ifndef DATA_GENERATOR_CLI_SHARED_H
#define DATA_GENERATOR_CLI_SHARED_H

#include <cxxopts.hpp>
#include <nlohmann/json.hpp>

#include <ostream>
#include <string>
#include <vector>

#include "config/generator_catalog.h"
#include "config/configuration.h"

namespace data_generator::cli {

using Json = nlohmann::json;
using OrderedJson = nlohmann::ordered_json;

Json load_json_from_file(const std::string& path);

OrderedJson to_ordered_json(const Json& value);

OrderedJson build_ordered_config_template(const config::GeneratorMetadata& meta);

cxxopts::ParseResult parse_options(cxxopts::Options& options, const std::vector<std::string>& args);

void print_validation_issues(const std::vector<config::ValidationIssue>& issues, std::ostream& output);

std::string version_string();

void print_version(std::ostream& output);

}  // namespace data_generator::cli

#endif
