// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file command_help.cpp

#include "cli/command_help.h"

#include <iostream>

#include "cli/cli_shared.h"
#include "cli/exit_codes.h"

namespace datagen::cli {

int CommandHelp::run(const std::vector<std::string>& /*args*/) {
    std::cout
        << program_display_name() << " CLI\n"
        << "\n"
        << "Commands:\n"
        << "  init       Generate JSON configuration template.\n"
        << "  preview    Generate preview rows from JSON config.\n"
        << "  run        Generate dataset from JSON config.\n"
        << "  info       List or describe generators.\n"
        << "  drivers    List installed ODBC drivers.\n"
        << "  check      Validate JSON configuration.\n"
        << "  schema     Generate JSON Schema from metadata.\n"
        << "  help       Show this help message.\n"
        << "\n"
        << "Command usage:\n"
        << "  " << program_display_name() << " init <json> [--template file|database] [--format <format>] [--from-database <connection>] "
           "[--table <name>]\n"
        << "  " << program_display_name() << " preview <json> [--rows <N>] [--field <name>]\n"
        << "  " << program_display_name() << " run <json> [--rows <N>] [--output <file>]\n"
        << "  " << program_display_name() << " info [<name>] [--json]\n"
        << "  " << program_display_name() << " drivers [--json]\n"
        << "  " << program_display_name() << " check <json>\n"
        << "  " << program_display_name() << " schema <file>\n"
        << "\n"
        << "Examples:\n"
        << "  " << program_display_name() << " init template.json\n"
        << "  " << program_display_name() << " init template.json --template file --format csv\n"
        << "  " << program_display_name() << " init template.json --template database --from-database \"odbc://DRIVER={MySQL ODBC 8.0 "
           "Driver};SERVER=127.0.0.1;PORT=3306;DATABASE=example_db;UID=user;PWD=password;\" --table t_orders\n"
        << "  " << program_display_name() << " preview config.json --rows 5\n"
        << "  " << program_display_name() << " preview config.json --field email\n"
        << "  " << program_display_name() << " run config.json\n"
        << "  " << program_display_name() << " run config.json --output ./out.sql\n"
        << "  " << program_display_name() << " drivers --json\n"
        << "  " << program_display_name() << " info sequence --json\n"
        << "  " << program_display_name() << " info\n"
        << "  " << program_display_name() << " check config.json\n"
        << "  " << program_display_name() << " schema schema/datagen.schema.json\n";
    return exit_codes::kOk;
}

}  // namespace datagen::cli
