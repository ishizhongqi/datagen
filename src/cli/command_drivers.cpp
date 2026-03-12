// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file command_drivers.cpp

#include "cli/command_drivers.h"

#include <cxxopts.hpp>
#include <iostream>

#include "cli/cli_shared.h"
#include "cli/exit_codes.h"
#include "output/database/odbc_registry.h"

namespace data_generator::cli {

int CommandDrivers::run(const std::vector<std::string>& args) {
    cxxopts::Options options("data-generator drivers", "List installed ODBC drivers.");
    options.add_options()
        ("json", "Output in JSON format")
        ("h,help", "Show help");

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

    std::vector<database::OdbcDriverInfo> drivers;
    std::string error;
    if (!database::list_odbc_drivers(&drivers, &error)) {
        std::cerr << "Failed to list ODBC drivers: " << error << "\n";
        return exit_codes::kRuntimeFailure;
    }

    if (result.count("json")) {
        Json output = Json::array();
        for (const auto& driver : drivers) {
            output.push_back(Json{{"name", driver.name}, {"attributes", driver.attributes}});
        }
        std::cout << output.dump(2) << "\n";
        return exit_codes::kOk;
    }

    if (drivers.empty()) {
        std::cout << "No ODBC drivers found.\n";
        return exit_codes::kOk;
    }

    for (const auto& driver : drivers) {
        std::cout << driver.name << "\n";
    }
    return exit_codes::kOk;
}

}  // namespace data_generator::cli
