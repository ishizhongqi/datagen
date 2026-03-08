// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file cli_dispatcher.cpp

#include "cli/cli_dispatcher.h"

#include <exception>
#include <iostream>

#include "cli/command_describe.h"
#include "cli/command_drivers.h"
#include "cli/command_generate.h"
#include "cli/command_help.h"
#include "cli/command_init.h"
#include "cli/command_list.h"
#include "cli/command_preview.h"
#include "cli/command_schema.h"
#include "cli/command_validate.h"
#include "cli/exit_codes.h"

namespace data_generator::cli {

int CliDispatcher::dispatch(const std::string& command, const std::vector<std::string>& args) {
    try {
        if (command == "init") { return CommandInit::run(args); }
        if (command == "preview") { return CommandPreview::run(args); }
        if (command == "generate") { return CommandGenerate::run(args); }
        if (command == "describe") { return CommandDescribe::run(args); }
        if (command == "drivers") { return CommandDrivers::run(args); }
        if (command == "list") { return CommandList::run(args); }
        if (command == "validate") { return CommandValidate::run(args); }
        if (command == "schema") { return CommandSchema::run(args); }
        if (command.empty() || command == "help") { return CommandHelp::run(args); }

        std::cerr << "Unknown command: " << command << "\n";
        return CommandHelp::run({});
    } catch (const std::exception& ex) {
        std::cerr << "CLI error: " << ex.what() << "\n";
        return exit_codes::kCliError;
    }
}

}  // namespace data_generator::cli
