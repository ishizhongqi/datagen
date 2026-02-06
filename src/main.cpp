// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file main.cpp

#include <fstream>
#include <iomanip>
#include <iostream>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <vector>
#include <cxxopts.hpp>

#include "generators/business_generators.h"
#include "generators/computer_generators.h"
#include "generators/datetime_generators.h"
#include "generators/generator_registry.h"
#include "generators/location_generators.h"
#include "generators/number_generators.h"
#include "generators/payment_generators.h"
#include "generators/person_generators.h"
#include "generators/product_generators.h"
#include "generators/string_generators.h"
#include "generators/utility_generators.h"

namespace data_generator {

using Json = nlohmann::json;
using generator::GeneratorRegistry;
using generator::IGenerator;

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

void run(const std::string& input_path) {
    GeneratorRegistry registry;
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

    std::ifstream input_stream(input_path);
    if (!input_stream) { throw std::runtime_error("Failed to open input json: " + input_path); }
    Json root;
    input_stream >> root;

    std::vector<std::pair<std::string, std::unique_ptr<IGenerator>>> fields;

    for (const auto& col : root["fields"]) {
        const std::string name      = col.at("name").get<std::string>();
        const std::string generator = col.at("generator").get<std::string>();

        Json col_with_rows    = col;
        col_with_rows["rows"] = root["rows"];
        if (root.contains("null_value_string")) { col_with_rows["null_value_string"] = root["null_value_string"]; }

        try {
            auto gen = registry.create(generator, col_with_rows);
            fields.emplace_back(name, std::move(gen));
        } catch (const std::exception& ex) {
            std::cerr << "Skip unknown generator '" << generator << "' for field '" << name << "': " << ex.what()
                      << "\n";
        }
    }

    const std::string output_format = root.value("output_format", "csv");
    std::string       output_path;
    if (output_format == "csv") {
        output_path = R"(D:\Projects\data_generator_bak\draft\output.csv)";
    } else if (output_format == "sql") {
        output_path = R"(D:\Projects\data_generator_bak\draft\output.sql)";
    } else if (output_format == "json") {
        output_path = R"(D:\Projects\data_generator_bak\draft\output.json)";
    } else {
        throw std::runtime_error("Unsupported output_format: " + output_format);
    }

    std::ofstream output_stream(output_path, std::ios::trunc);
    if (!output_stream) { throw std::runtime_error("Failed to open output file: " + output_path); }

    if (output_format == "csv") {
        for (size_t i = 0; i < fields.size(); ++i) {
            if (i > 0) { output_stream << ","; }
            output_stream << csv_escape(fields[i].first);
        }
        output_stream << "\n";

        for (int row = 0; row < root["rows"].get<int>(); ++row) {
            for (size_t i = 0; i < fields.size(); ++i) {
                if (i > 0) { output_stream << ","; }
                output_stream << csv_escape(fields[i].second->generate());
            }
            output_stream << "\n";
            for (auto& [_, gen] : fields) { gen->next(); }
        }
    } else if (output_format == "json") {
        output_stream << "[\n";
        for (int row = 0; row < root["rows"].get<int>(); ++row) {
            Json row_obj = Json::object();
            for (auto& [name, gen] : fields) { row_obj[name] = gen->generate(); }
            output_stream << "  " << row_obj.dump();
            if (row + 1 < root["rows"].get<int>()) { output_stream << ","; }
            output_stream << "\n";
            for (auto& [_, gen] : fields) { gen->next(); }
        }
        output_stream << "]\n";
    } else if (output_format == "sql") {
        const std::string table_name           = root.value("table_name", "generated_data");
        const bool        include_create_table = root.value("include_create_table", true);
        if (include_create_table) {
            output_stream << "CREATE TABLE " << table_name << " (\n";
            for (size_t i = 0; i < fields.size(); ++i) {
                output_stream << "  " << fields[i].first << " TEXT";
                if (i + 1 < fields.size()) { output_stream << ","; }
                output_stream << "\n";
            }
            output_stream << ");\n";
        }

        for (int row = 0; row < root["rows"].get<int>(); ++row) {
            output_stream << "INSERT INTO " << table_name << " (";
            for (size_t i = 0; i < fields.size(); ++i) {
                if (i > 0) { output_stream << ", "; }
                output_stream << fields[i].first;
            }
            output_stream << ") VALUES (";
            for (size_t i = 0; i < fields.size(); ++i) {
                if (i > 0) { output_stream << ", "; }
                const std::string value = fields[i].second->generate();
                output_stream << "'" << sql_escape(value) << "'";
            }
            output_stream << ");\n";
            for (auto& [_, gen] : fields) { gen->next(); }
        }
    } else {
        throw std::runtime_error("output_format not implemented yet: " + output_format);
    }
}

}  // namespace data_generator

int main(int argc, char** argv) {
    const std::string default_input = R"(D:\Projects\data_generator_bak\draft\input_example.json)";
    const std::string input_path    = argc > 1 ? argv[1] : default_input;
    data_generator::run(input_path);
    return 0;
}
