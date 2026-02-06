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
              << "  validate   Validate JSON configuration.\n"
              << "  help       Show this help message.\n"
              << "\n"
              << "Command usage:\n"
              << "  data-generator init [--generator <name>] [--output <file>]\n"
              << "  data-generator preview --input <json> [--field <name>|all]\n"
              << "  data-generator generate --input <json> [--rows <N>] [--output <file>] [--format <json|csv|sql>]\n"
              << "  data-generator describe --generator <name>\n"
              << "  data-generator validate --input <json>\n"
              << "\n"
              << "Examples:\n"
              << "  data-generator init\n"
              << "  data-generator init --generator email --output template.json\n"
              << "  data-generator preview --input config.json\n"
              << "  data-generator preview --input config.json --field email\n"
              << "  data-generator generate --input config.json --rows 1000 --format csv\n"
              << "  data-generator generate --input config.json --output out.json --format json\n"
              << "  data-generator describe --generator sequence\n"
              << "  data-generator validate --input config.json\n";
    return 0;
}

}  // namespace data_generator::cli
