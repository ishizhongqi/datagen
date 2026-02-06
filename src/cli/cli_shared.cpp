// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file cli_shared.cpp

#include "cli/cli_shared.h"

#include <fstream>
#include <stdexcept>
#include <string>
#include <utility>

#include "generators/business_generators.h"
#include "generators/computer_generators.h"
#include "generators/datetime_generators.h"
#include "generators/location_generators.h"
#include "generators/number_generators.h"
#include "generators/payment_generators.h"
#include "generators/person_generators.h"
#include "generators/product_generators.h"
#include "generators/string_generators.h"
#include "generators/utility_generators.h"

namespace data_generator::cli {

namespace {

constexpr int         kDefaultRows = 100;
constexpr const char* kFieldsKey   = "fields";

}  // namespace

generator::GeneratorRegistry build_registry() {
    generator::GeneratorRegistry registry;
    register_computer_generators(registry);
    register_business_generators(registry);
    register_datetime_generators(registry);
    register_location_generators(registry);
    register_number_generators(registry);
    register_payment_generators(registry);
    register_person_generators(registry);
    register_product_generators(registry);
    register_string_generators(registry);
    register_utility_generators(registry);
    return registry;
}

Json load_json_from_file(const std::string& path) {
    std::ifstream input_stream(path);
    if (!input_stream) { throw std::runtime_error("Failed to open input json: " + path); }
    Json root;
    input_stream >> root;
    return root;
}

int resolve_row_count(const Json& root, int cli_rows, int default_rows) {
    if (cli_rows > 0) { return cli_rows; }
    if (root.contains("rows") && root.at("rows").is_number_integer()) { return root.at("rows").get<int>(); }
    return default_rows > 0 ? default_rows : kDefaultRows;
}

std::vector<FieldGenerator> build_field_generators(const Json& root, int rows) {
    if (!root.contains(kFieldsKey) || !root.at(kFieldsKey).is_array()) {
        throw std::runtime_error("Input json must contain array field: fields");
    }

    const generator::GeneratorRegistry registry = build_registry();
    std::vector<FieldGenerator>        fields;

    for (const auto& field : root.at(kFieldsKey)) {
        if (!field.is_object()) { throw std::runtime_error("Each field must be a json object"); }
        if (!field.contains("name") || !field.at("name").is_string()) {
            throw std::runtime_error("Each field must contain a string name");
        }
        if (!field.contains("generator") || !field.at("generator").is_string()) {
            throw std::runtime_error("Each field must contain a string generator");
        }
        if (!field.contains("config") || !field.at("config").is_object()) {
            throw std::runtime_error("Each field must contain object config");
        }

        Json field_with_rows    = field;
        field_with_rows["rows"] = rows;
        if (root.contains("null_value_string")) { field_with_rows["null_value_string"] = root["null_value_string"]; }

        const std::string name           = field.at("name").get<std::string>();
        const std::string generator_name = field.at("generator").get<std::string>();
        auto              generator      = registry.create(generator_name, field_with_rows);
        fields.push_back({name, std::move(generator)});
    }

    return fields;
}

std::string csv_escape(const std::string& value) {
    if (value.find_first_of(",\"\n\r") == std::string::npos) { return value; }
    std::string escaped;
    escaped.reserve(value.size() + 2);
    escaped.push_back('"');
    for (const char ch : value) {
        if (ch == '"') { escaped.push_back('"'); }
        escaped.push_back(ch);
    }
    escaped.push_back('"');
    return escaped;
}

std::string sql_escape(const std::string& value) {
    std::string escaped;
    escaped.reserve(value.size());
    for (const char ch : value) {
        if (ch == '\'') { escaped.push_back('\''); }
        escaped.push_back(ch);
    }
    return escaped;
}

cxxopts::ParseResult parse_options(cxxopts::Options& options, const std::vector<std::string>& args) {
    std::vector<std::string> arg_storage;
    arg_storage.reserve(args.size() + 1);
    arg_storage.push_back(options.program());
    arg_storage.insert(arg_storage.end(), args.begin(), args.end());

    std::vector<char*> argv;
    argv.reserve(arg_storage.size());
    for (auto& arg : arg_storage) { argv.push_back(const_cast<char*>(arg.c_str())); }

    return options.parse(static_cast<int>(argv.size()), argv.data());
}

}  // namespace data_generator::cli
