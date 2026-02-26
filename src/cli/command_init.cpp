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
#include "cli/exit_codes.h"
#include "cli/generator_catalog.h"

namespace data_generator::cli {

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
        return exit_codes::kUsage;
    }

    if (result.count("help")) {
        std::cout << options.help() << "\n";
        return exit_codes::kOk;
    }

    int         rows             = 100;
    std::string output_format    = "csv";
    const bool  rows_specified   = result.count("rows") > 0;
    const bool  format_specified = result.count("format") > 0;
    if (rows_specified) {
        rows = result["rows"].as<int>();
        if (rows <= 0) {
            std::cerr << "rows must be a positive integer.\n";
            return exit_codes::kUsage;
        }
    }
    if (format_specified) {
        output_format = result["format"].as<std::string>();
        if (output_format != "csv" && output_format != "json" && output_format != "sql") {
            std::cerr << "Unsupported format: " << output_format << "\n";
            return exit_codes::kUsage;
        }
    }

    OrderedJson output = OrderedJson::object();
    if (result.count("generator")) {
        const std::string        generator_name = result["generator"].as<std::string>();
        const GeneratorMetadata* meta           = find_generator_metadata(generator_name);
        if (!meta) {
            std::cerr << "Unknown generator: " << generator_name << "\n";
            return exit_codes::kUsage;
        }
        output["generator"]             = meta->name;
        output["config"]                = build_ordered_config_template(*meta);
        output["supports_unique"]       = meta->supports_unique;
        output["supports_data_linkage"] = meta->supports_data_linkage;
        if (meta->supports_unique) { output["unique"] = false; }
        if (meta->supports_data_linkage) {
            if (!meta->linkage_module.empty()) {
                output["data_linkage"] = meta->linkage_module + ":group_1";
            } else {
                output["data_linkage"] = false;
            }
        }
        if (rows_specified) { output["rows"] = rows; }
        if (format_specified) {
            output["output_format"] = output_format;
            if (output_format == "sql") {
                output["table_name"]           = "generated_data";
                output["include_create_table"] = true;
            }
        }
    } else {
        const Json root = build_project_template(rows, output_format);
        output["$schema"]           = "./schema/data-generator.schema.json";
        output["rows"]              = root.at("rows");
        output["output_format"]     = root.at("output_format");
        output["null_value_string"] = to_ordered_json(root.at("null_value_string"));
        if (root.contains("table_name")) { output["table_name"] = root.at("table_name"); }
        if (root.contains("include_create_table")) { output["include_create_table"] = root.at("include_create_table"); }

        OrderedJson fields = OrderedJson::array();
        if (root.contains("fields") && root.at("fields").is_array()) {
            for (const auto& field : root.at("fields")) {
                OrderedJson ordered_field = OrderedJson::object();
                if (field.contains("name")) { ordered_field["name"] = field.at("name"); }
                if (field.contains("generator")) { ordered_field["generator"] = field.at("generator"); }
                if (field.contains("config")) { ordered_field["config"] = to_ordered_json(field.at("config")); }
                if (field.contains("unique")) { ordered_field["unique"] = field.at("unique"); }
                if (field.contains("data_linkage")) { ordered_field["data_linkage"] = field.at("data_linkage"); }
                fields.push_back(std::move(ordered_field));
            }
        }
        output["fields"] = std::move(fields);
    }

    const std::string payload = output.dump(2);
    if (result.count("output")) {
        const std::string path = result["output"].as<std::string>();
        std::ofstream     out(path, std::ios::trunc);
        if (!out) {
            std::cerr << "Failed to open output file: " << path << "\n";
            return exit_codes::kCliError;
        }
        out << payload << "\n";
        return exit_codes::kOk;
    }

    std::cout << payload << "\n";
    return exit_codes::kOk;
}

}  // namespace data_generator::cli
