// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file command_validate.cpp

#include "cli/command_validate.h"

#include <cxxopts.hpp>
#include <iostream>
#include <set>
#include <string>
#include <vector>

#include "cli/cli_shared.h"
#include "cli/generator_catalog.h"
#include "generators/linkage_helper.h"

namespace data_generator::cli {

namespace {

constexpr int kExitOk      = 0;
constexpr int kExitUsage   = 2;
constexpr int kExitInvalid = 3;

bool is_valid_output_format(const Json& root) {
    if (!root.contains("output_format")) { return true; }
    if (!root.at("output_format").is_string()) { return false; }
    const std::string value = root.at("output_format").get<std::string>();
    return value == "csv" || value == "json" || value == "sql";
}

}  // namespace

int CommandValidate::run(const std::vector<std::string>& args) {
    cxxopts::Options options("data-generator validate", "Validate JSON configuration.");
    options.add_options()("input", "Input JSON file", cxxopts::value<std::string>())("h,help", "Show help");

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

    std::vector<std::string> errors;
    Json                     root;
    try {
        root = load_json_from_file(result["input"].as<std::string>());
    } catch (const std::exception& ex) {
        std::cerr << "Failed to load json: " << ex.what() << "\n";
        return kExitInvalid;
    }

    if (!root.is_object()) { errors.emplace_back("Root must be a json object."); }
    if (root.contains("rows") && !root.at("rows").is_number_integer()) {
        errors.emplace_back("rows must be an integer.");
    }
    if (!is_valid_output_format(root)) { errors.emplace_back("output_format must be one of csv, json, sql."); }
    if (!root.contains("fields") || !root.at("fields").is_array()) { errors.emplace_back("fields must be an array."); }

    if (errors.empty()) {
        const auto& fields = root.at("fields");
        for (size_t index = 0; index < fields.size(); ++index) {
            const auto& field = fields.at(index);
            if (!field.is_object()) {
                errors.emplace_back("fields[" + std::to_string(index) + "] must be an object.");
                continue;
            }
            if (!field.contains("name") || !field.at("name").is_string()) {
                errors.emplace_back("fields[" + std::to_string(index) + "].name must be a string.");
            }
            if (!field.contains("generator") || !field.at("generator").is_string()) {
                errors.emplace_back("fields[" + std::to_string(index) + "].generator must be a string.");
                continue;
            }
            if (!field.contains("config") || !field.at("config").is_object()) {
                errors.emplace_back("fields[" + std::to_string(index) + "].config must be an object.");
                continue;
            }

            const std::string        generator_name = field.at("generator").get<std::string>();
            const GeneratorMetadata* meta           = find_generator_metadata(generator_name);
            if (!meta) {
                errors.emplace_back("fields[" + std::to_string(index) + "]: unknown generator " + generator_name);
                continue;
            }

            if (field.contains("unique")) {
                if (!field.at("unique").is_boolean()) {
                    errors.emplace_back("fields[" + std::to_string(index) + "].unique must be a boolean.");
                } else if (!meta->supports_unique) {
                    errors.emplace_back("fields[" + std::to_string(index) + "]: generator does not support unique.");
                }
            }

            if (field.contains("data_linkage")) {
                try {
                    data_generator::generator::parse_linkage_key(field);
                } catch (const std::exception& ex) {
                    errors.emplace_back(
                        "fields[" + std::to_string(index) + "].data_linkage invalid: " + std::string(ex.what())
                    );
                }
                if (!meta->supports_data_linkage) {
                    errors.emplace_back(
                        "fields[" + std::to_string(index) + "]: generator does not support data_linkage."
                    );
                }
            }

            std::set<std::string> allowed_keys;
            for (const auto& param : meta->config_params) { allowed_keys.insert(param.name); }
            for (auto it = field.at("config").begin(); it != field.at("config").end(); ++it) {
                if (allowed_keys.find(it.key()) == allowed_keys.end()) {
                    errors.emplace_back(
                        "fields[" + std::to_string(index) + "].config contains unknown key: " + it.key()
                    );
                }
            }
        }
    }

    if (!errors.empty()) {
        for (const auto& error : errors) { std::cerr << "Validation error: " << error << "\n"; }
        return kExitInvalid;
    }

    try {
        const int rows = resolve_row_count(root, 0, 100);
        build_field_generators(root, rows);
    } catch (const std::exception& ex) {
        std::cerr << "Validation error: " << ex.what() << "\n";
        return kExitInvalid;
    }

    std::cout << "Validation success.\n";
    return kExitOk;
}

}  // namespace data_generator::cli
