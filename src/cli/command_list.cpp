// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file command_list.cpp

#include "cli/command_list.h"

#include <algorithm>
#include <cxxopts.hpp>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "cli/cli_shared.h"
#include "cli/exit_codes.h"
#include "cli/generator_catalog.h"

namespace data_generator::cli {

int CommandList::run(const std::vector<std::string>& args) {
    cxxopts::Options options("data-generator list", "List available generators.");
    options.add_options()("h,help", "Show help");

    cxxopts::ParseResult result;
    try {
        result = parse_options(options, args);
    } catch (const std::exception& ex) {
        std::cerr << "Failed to parse arguments: " << ex.what() << "\n";
        std::cerr << options.help() << "\n";
        return exit_codes::kUsage;
    }

    if (result.count("help")) {
        std::cout << options.help() << "\n";
        return exit_codes::kOk;
    }

    const auto& catalog = get_generator_catalog();

    std::cout << "Available Generators:\n";

    std::map<std::string, std::vector<std::string>> grouped;
    for (const auto& meta : catalog) { grouped[meta.module].push_back(meta.name); }

    bool first_group = true;
    for (auto& [module, names] : grouped) {
        std::sort(names.begin(), names.end());
        if (!first_group) { std::cout << "\n"; }
        first_group = false;
        std::cout << "[" << module << "]\n";
        for (const auto& name : names) { std::cout << "- " << name << "\n"; }
    }

    return exit_codes::kOk;
}

}  // namespace data_generator::cli
