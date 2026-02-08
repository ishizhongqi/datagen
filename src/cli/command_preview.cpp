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

bool is_null_literal(const Json& root, const std::string& value) {
    if (!root.contains("null_value_string")) { return false; }
    if (root.at("null_value_string").is_null()) { return value.empty(); }
    return false;
}

}  // namespace

int CommandPreview::run(const std::vector<std::string>& args) {
    cxxopts::Options options("data-generator preview", "Generate a single row preview.");
    options.add_options()
        ("input", "Input JSON file", cxxopts::value<std::string>())
        ("field", "Field name or 'all'", cxxopts::value<std::string>()->default_value("all"))
        ("format", "Output format (json|csv|sql)", cxxopts::value<std::string>()->default_value("csv"))
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
        const std::string format       = result["format"].as<std::string>();
        const Json        root         = load_json_from_file(input_path);

        std::vector<std::string> errors;
        if (!validate_config(root, false, &errors)) {
            print_validation_errors(errors, std::cerr);
            return kExitFailed;
        }

        if (format != "csv" && format != "json" && format != "sql") {
            std::cerr << "Unsupported format: " << format << "\n";
            return kExitUsage;
        }

        constexpr int rows   = 1;
        auto          fields = build_field_generators(root, rows);

        if (field_filter != "all" && !field_filter.empty()) {
            for (auto& field : fields) {
                if (field.name == field_filter) {
                    std::cout << field.generator->generate() << "\n";
                    return kExitOk;
                }
            }
            std::cerr << "Field not found: " << field_filter << "\n";
            return kExitFailed;
        }

        if (format == "json") {
            Json row = Json::object();
            for (auto& field : fields) {
                const std::string value = field.generator->generate();
                if (is_null_literal(root, value)) {
                    row[field.name] = nullptr;
                } else {
                    row[field.name] = value;
                }
            }
            std::cout << row.dump(2) << "\n";
            return kExitOk;
        }

        if (format == "csv") {
            for (size_t i = 0; i < fields.size(); ++i) {
                if (i > 0) { std::cout << ","; }
                std::cout << csv_escape(fields[i].generator->generate());
            }
            std::cout << "\n";
            return kExitOk;
        }

        std::cout << "INSERT INTO preview_table (";
        for (size_t i = 0; i < fields.size(); ++i) {
            if (i > 0) { std::cout << ", "; }
            std::cout << fields[i].name;
        }
        std::cout << ") VALUES (";
        for (size_t i = 0; i < fields.size(); ++i) {
            if (i > 0) { std::cout << ", "; }
            const std::string value = fields[i].generator->generate();
            if (is_null_literal(root, value)) {
                std::cout << "NULL";
            } else {
                std::cout << "'" << sql_escape(value) << "'";
            }
        }
        std::cout << ");\n";
        return kExitOk;
    } catch (const std::exception& ex) {
        std::cerr << "Preview failed: " << ex.what() << "\n";
        return kExitFailed;
    }
}

}  // namespace data_generator::cli
