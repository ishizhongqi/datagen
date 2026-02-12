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

#include "cli/generator_catalog.h"
#include "core/configuration.h"

namespace data_generator::cli {

using Json = nlohmann::json;

Json load_json_from_file(const std::string& path);

cxxopts::ParseResult parse_options(cxxopts::Options& options, const std::vector<std::string>& args);

void print_validation_issues(const std::vector<core::ValidationIssue>& issues, std::ostream& output);

std::vector<std::string> build_describe_text_lines(const GeneratorMetadata& meta);

}  // namespace data_generator::cli

#endif
