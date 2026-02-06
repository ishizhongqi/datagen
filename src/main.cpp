// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file main.cpp

#include <cxxopts.hpp>
#include <string>
#include <vector>

#include "cli/cli_dispatcher.h"

int main(int argc, char** argv) {
    cxxopts::Options options("data-generator", "Data generator CLI.");
    options.allow_unrecognised_options();
    options.add_options()("command", "Command to run", cxxopts::value<std::string>()->default_value(""));
    options.parse_positional({"command"});

    const auto                      result   = options.parse(argc, argv);
    const std::string               command  = result["command"].as<std::string>();
    const std::vector<std::string>& sub_args = result.unmatched();

    return data_generator::cli::CliDispatcher::dispatch(command, sub_args);
}
