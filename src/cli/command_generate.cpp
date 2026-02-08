// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file command_generate.cpp

#include "cli/command_generate.h"

#include <cxxopts.hpp>
#include <fstream>
#include <iostream>
#include <string>

#include "cli/cli_shared.h"

namespace data_generator::cli {

namespace {

constexpr int kExitOk     = 0;
constexpr int kExitUsage  = 2;
constexpr int kExitFailed = 3;

}  // namespace

int CommandGenerate::run(const std::vector<std::string>& args) {
    cxxopts::Options options("data-generator generate", "Generate dataset from JSON config.");
    options.add_options()
        ("input", "Input JSON file", cxxopts::value<std::string>())
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

    if (!result.count("input")) {
        std::cerr << "Missing required --input\n";
        std::cerr << options.help() << "\n";
        return kExitUsage;
    }

    try {
        const std::string input_path = result["input"].as<std::string>();
        const Json root = load_json_from_file(input_path);
        std::vector<std::string> errors;
        if (!validate_config(root, true, &errors)) {
            print_validation_errors(errors, std::cerr);
            return kExitFailed;
        }

        const int         rows   = root.at("rows").get<int>();
        const std::string format = root.at("output_format").get<std::string>();
        auto              fields = build_field_generators(root, rows);

        std::ostream* output = &std::cout;
        std::ofstream output_stream;
        if (result.count("output")) {
            const std::string output_path = result["output"].as<std::string>();
            output_stream.open(output_path, std::ios::trunc);
            if (!output_stream) {
                std::cerr << "Failed to open output file: " << output_path << "\n";
                return kExitFailed;
            }
            output = &output_stream;
        }

        if (format == "csv") {
            for (size_t i = 0; i < fields.size(); ++i) {
                if (i > 0) { *output << ","; }
                *output << csv_escape(fields[i].name);
            }
            *output << "\n";

            for (int row = 0; row < rows; ++row) {
                for (size_t i = 0; i < fields.size(); ++i) {
                    if (i > 0) { *output << ","; }
                    *output << csv_escape(fields[i].generator->generate());
                }
                *output << "\n";
                for (auto& field : fields) { field.generator->next(); }
            }
        } else if (format == "json") {
            *output << "[\n";
            for (int row = 0; row < rows; ++row) {
                Json row_obj = Json::object();
                for (auto& field : fields) { row_obj[field.name] = field.generator->generate(); }
                *output << "  " << row_obj.dump();
                if (row + 1 < rows) { *output << ","; }
                *output << "\n";
                for (auto& field : fields) { field.generator->next(); }
            }
            *output << "]\n";
        } else if (format == "sql") {
            const std::string table_name           = root.value("table_name", "generated_data");
            const bool        include_create_table = root.value("include_create_table", true);
            if (include_create_table) {
                *output << "CREATE TABLE " << table_name << " (\n";
                for (size_t i = 0; i < fields.size(); ++i) {
                    *output << "  " << fields[i].name << " TEXT";
                    if (i + 1 < fields.size()) { *output << ","; }
                    *output << "\n";
                }
                *output << ");\n";
            }

            for (int row = 0; row < rows; ++row) {
                *output << "INSERT INTO " << table_name << " (";
                for (size_t i = 0; i < fields.size(); ++i) {
                    if (i > 0) { *output << ", "; }
                    *output << fields[i].name;
                }
                *output << ") VALUES (";
                for (size_t i = 0; i < fields.size(); ++i) {
                    if (i > 0) { *output << ", "; }
                    const std::string value = fields[i].generator->generate();
                    *output << "'" << sql_escape(value) << "'";
                }
                *output << ");\n";
                for (auto& field : fields) { field.generator->next(); }
            }
        }

        return kExitOk;
    } catch (const std::exception& ex) {
        std::cerr << "Generate failed: " << ex.what() << "\n";
        return kExitFailed;
    }
}

}  // namespace data_generator::cli
