// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file run.cpp

#include "app/run.h"

#include <cxxopts.hpp>
#include <iostream>
#include <string>
#include <vector>

#include "cli/cli_dispatcher.h"
#include "cli/cli_shared.h"
#include "cli/command_help.h"
#include "cli/exit_codes.h"

namespace datagen {

int run(int argc, char** argv) {
    if (argc >= 2) {
        const std::string first_arg = argv[1];
        if (first_arg == "-h" || first_arg == "--help") { return cli::CommandHelp::run({}); }
        if (first_arg == "-v" || first_arg == "-V" || first_arg == "--version" || first_arg == "version") {
            cli::print_version(std::cout);
            return cli::exit_codes::kOk;
        }
    }

    cxxopts::Options options(cli::program_display_name(), "Datagen CLI.");
    options.allow_unrecognised_options();
    options.add_options()("command", "Command to run", cxxopts::value<std::string>()->default_value(""));
    options.parse_positional({"command"});

    const auto                      result   = options.parse(argc, argv);
    const std::string               command  = result["command"].as<std::string>();
    const std::vector<std::string>& sub_args = result.unmatched();

    return cli::CliDispatcher::dispatch(command, sub_args);
}

}  // namespace datagen
