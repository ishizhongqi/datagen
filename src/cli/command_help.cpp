// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file command_help.cpp

#include "cli/command_help.h"

#include <iostream>

#include "cli/exit_codes.h"

namespace data_generator::cli {

int CommandHelp::run(const std::vector<std::string>& /*args*/) {
    std::cout
        << "data-generator CLI\n"
        << "\n"
        << "Commands:\n"
        << "  init       Generate JSON configuration template.\n"
        << "  preview    Generate a single row preview from JSON config.\n"
        << "  run        Generate dataset from JSON config.\n"
        << "  info       List or describe generators.\n"
        << "  drivers    List installed ODBC drivers.\n"
        << "  check      Validate JSON configuration.\n"
        << "  schema     Generate JSON Schema from metadata.\n"
        << "  help       Show this help message.\n"
        << "\n"
        << "Command usage:\n"
        << "  data-generator init <json> [--template file|database] [--format <format>] [--from-database <connection>] "
           "[--table <name>]\n"
        << "  data-generator preview <json> [--field <name>]\n"
        << "  data-generator run <json> [--rows <N>] [--output <file>]\n"
        << "  data-generator info [<name>] [--json]\n"
        << "  data-generator drivers [--json]\n"
        << "  data-generator check <json>\n"
        << "  data-generator schema <file>\n"
        << "\n"
        << "Examples:\n"
        << "  data-generator init template.json\n"
        << "  data-generator init template.json --template file --format csv\n"
        << "  data-generator init template.json --template database --from-database \"odbc://DRIVER={MySQL ODBC 8.0 "
           "Driver};SERVER=127.0.0.1;PORT=3306;DATABASE=example_db;UID=user;PWD=password;\" --table t_orders\n"
        << "  data-generator preview config.json\n"
        << "  data-generator preview config.json --field email\n"
        << "  data-generator run config.json\n"
        << "  data-generator run config.json --output ./out.sql\n"
        << "  data-generator drivers --json\n"
        << "  data-generator info sequence --json\n"
        << "  data-generator info\n"
        << "  data-generator check config.json\n"
        << "  data-generator schema schema/data-generator.schema.json\n";
    return exit_codes::kOk;
}

}  // namespace data_generator::cli
