// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file command_help.cpp

#include "cli/command_help.h"

#include <iostream>

namespace data_generator::cli {

int CommandHelp::run(const std::vector<std::string>& /*args*/) {
    std::cout << "data-generator CLI\n"
              << "\n"
              << "Commands:\n"
              << "  init       Generate JSON configuration template.\n"
              << "  preview    Generate a single row preview from JSON config.\n"
        << "  generate   Generate dataset from JSON config.\n"
        << "  describe   Show generator metadata.\n"
        << "  list       List available generators.\n"
        << "  validate   Validate JSON configuration.\n"
        << "  help       Show this help message.\n"
        << "\n"
        << "Command usage:\n"
        << "  data-generator init [--generator <name>] [--rows <N>] [--format <json|csv|sql>] [--output <file>]\n"
        << "  data-generator preview --input <json> [--field <name>|all] [--format <json|csv|sql>]\n"
        << "  data-generator generate --input <json> [--output <file>]\n"
        << "  data-generator describe --generator <name> [--json]\n"
        << "  data-generator list\n"
        << "  data-generator validate --input <json>\n"
        << "\n"
        << "Examples:\n"
        << "  data-generator init\n"
        << "  data-generator init --generator email --rows 100 --format csv --output template.json\n"
        << "  data-generator preview --input config.json\n"
        << "  data-generator preview --input config.json --field email\n"
        << "  data-generator preview --input config.json --format json\n"
        << "  data-generator generate --input config.json\n"
        << "  data-generator generate --input config.json --output out.json\n"
        << "  data-generator describe --generator sequence --json\n"
        << "  data-generator list\n"
        << "  data-generator validate --input config.json\n";
    return 0;
}

}  // namespace data_generator::cli
