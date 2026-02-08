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
#include "cli/generator_catalog.h"

namespace data_generator::cli {

namespace {

constexpr int kExitOk    = 0;
constexpr int kExitUsage = 2;

}  // namespace

int CommandList::run(const std::vector<std::string>& args) {
    cxxopts::Options options("data-generator list", "List available generators.");
    options.add_options()("h,help", "Show help");

    cxxopts::ParseResult result;
    try {
        result = parse_options(options, args);
    } catch (const std::exception& ex) {
        std::cerr << "Failed to parse arguments: " << ex.what() << "\n";
        std::cerr << options.help() << "\n";
        return kExitUsage;
    }

    if (result.count("help")) {
        std::cout << options.help() << "\n";
        return kExitOk;
    }

    const auto& catalog     = get_generator_catalog();
    bool        has_modules = true;
    for (const auto& meta : catalog) {
        if (meta.module.empty()) {
            has_modules = false;
            break;
        }
    }

    std::cout << "Available Generators:\n";

    if (!has_modules) {
        std::vector<std::string> names;
        names.reserve(catalog.size());
        for (const auto& meta : catalog) { names.push_back(meta.name); }
        std::sort(names.begin(), names.end());
        for (const auto& name : names) { std::cout << "- " << name << "\n"; }
        return kExitOk;
    }

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

    return kExitOk;
}

}  // namespace data_generator::cli
