// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file command_preview.cpp

#include "cli/command_preview.h"

#include <cxxopts.hpp>
#include <iostream>
#include <string>

#include "cli/cli_shared.h"

namespace data_generator::cli {

namespace {

constexpr int kExitOk     = 0;
constexpr int kExitUsage  = 2;
constexpr int kExitFailed = 3;

bool match_field(const std::string& filter, const std::string& name) {
    return filter.empty() || filter == "all" || filter == name;
}

}  // namespace

int CommandPreview::run(const std::vector<std::string>& args) {
    cxxopts::Options options("data-generator preview", "Generate a single row preview.");
    options.add_options()
        ("input", "Input JSON file", cxxopts::value<std::string>())
        ("field", "Field name or 'all'", cxxopts::value<std::string>()->default_value("all"))
        ("h,help", "Show help");

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

    if (!result.count("input")) {
        std::cerr << "Missing required --input\n";
        std::cerr << options.help() << "\n";
        return kExitUsage;
    }

    try {
        const std::string input_path   = result["input"].as<std::string>();
        const std::string field_filter = result["field"].as<std::string>();
        const Json        root         = load_json_from_file(input_path);

        constexpr int rows   = 1;
        auto          fields = build_field_generators(root, rows);

        Json row     = Json::object();
        bool matched = false;
        for (auto& field : fields) {
            const std::string value = field.generator->generate();
            if (match_field(field_filter, field.name)) {
                row[field.name] = value;
                matched         = true;
            }
        }
        if (!matched) {
            std::cerr << "Field not found: " << field_filter << "\n";
            return kExitFailed;
        }
        std::cout << row.dump(2) << "\n";
        return kExitOk;
    } catch (const std::exception& ex) {
        std::cerr << "Preview failed: " << ex.what() << "\n";
        return kExitFailed;
    }
}

}  // namespace data_generator::cli
