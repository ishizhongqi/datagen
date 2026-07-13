// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file cli_shared.h

#ifndef DATAGEN_CLI_SHARED_H
#define DATAGEN_CLI_SHARED_H

#include <cxxopts.hpp>
#include <nlohmann/json.hpp>
#include <ostream>
#include <string>
#include <vector>

#include "config/configuration.h"
#include "config/generator_catalog.h"

namespace datagen::cli {

using Json        = nlohmann::json;
using OrderedJson = nlohmann::ordered_json;

Json load_json_from_file(const std::string& path);

OrderedJson to_ordered_json(const Json& value);

OrderedJson build_ordered_config_template(const config::GeneratorMetadata& meta);

cxxopts::ParseResult parse_options(cxxopts::Options& options, const std::vector<std::string>& args);

void print_validation_issues(const std::vector<config::ValidationIssue>& issues, std::ostream& output);

std::string version_string();
std::string program_display_name();
std::string github_url();

void print_version(std::ostream& output);

}  // namespace datagen::cli

#endif
