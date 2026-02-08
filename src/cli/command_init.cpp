// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file command_init.cpp

#include "cli/command_init.h"

#include <cxxopts.hpp>
#include <fstream>
#include <iostream>
#include <string>

#include "cli/cli_shared.h"
#include "cli/generator_catalog.h"

namespace data_generator::cli {

namespace {

constexpr int kExitOk    = 0;
constexpr int kExitUsage = 2;

}  // namespace

int CommandInit::run(const std::vector<std::string>& args) {
    cxxopts::Options options("data-generator init", "Generate JSON configuration template.");
    options.add_options()
        ("generator", "Generator name", cxxopts::value<std::string>())
        ("rows", "Number of rows", cxxopts::value<int>())
        ("format", "Output format (json|csv|sql)", cxxopts::value<std::string>())
        ("output", "Output file path", cxxopts::value<std::string>())
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

    int rows = 100;
    std::string output_format = "csv";
    const bool rows_specified = result.count("rows") > 0;
    const bool format_specified = result.count("format") > 0;
    if (rows_specified) {
        rows = result["rows"].as<int>();
        if (rows <= 0) {
            std::cerr << "rows must be a positive integer.\n";
            return kExitUsage;
        }
    }
    if (format_specified) {
        output_format = result["format"].as<std::string>();
        if (output_format != "csv" && output_format != "json" && output_format != "sql") {
            std::cerr << "Unsupported format: " << output_format << "\n";
            return kExitUsage;
        }
    }

    Json output;
    if (result.count("generator")) {
        const std::string        generator_name = result["generator"].as<std::string>();
        const GeneratorMetadata* meta           = find_generator_metadata(generator_name);
        if (!meta) {
            std::cerr << "Unknown generator: " << generator_name << "\n";
            return kExitUsage;
        }
        output["generator"]             = meta->name;
        output["config"]                = meta->config_template;
        output["supports_unique"]       = meta->supports_unique;
        output["supports_data_linkage"] = meta->supports_data_linkage;
        if (rows_specified) { output["rows"] = rows; }
        if (format_specified) {
            output["output_format"] = output_format;
            if (output_format == "sql") {
                output["table_name"] = "generated_data";
                output["include_create_table"] = true;
            }
        }
        if (meta->supports_unique) { output["unique"] = false; }
        if (meta->supports_data_linkage) {
            if (!meta->linkage_module.empty()) {
                output["data_linkage"] = meta->linkage_module + ":group_1";
            } else {
                output["data_linkage"] = false;
            }
        }
    } else {
        output = build_project_template(rows, output_format);
    }

    const std::string payload = output.dump(2);
    if (result.count("output")) {
        const std::string path = result["output"].as<std::string>();
        std::ofstream     out(path, std::ios::trunc);
        if (!out) {
            std::cerr << "Failed to open output file: " << path << "\n";
            return 1;
        }
        out << payload << "\n";
        return kExitOk;
    }

    std::cout << payload << "\n";
    return kExitOk;
}

}  // namespace data_generator::cli
