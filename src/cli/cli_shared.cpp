// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file cli_shared.cpp

#include "cli/cli_shared.h"

#include <fstream>
#include <stdexcept>
#include <unordered_set>
#include <utility>

namespace datagen::cli {

#ifndef DATAGEN_VERSION
#define DATAGEN_VERSION "0.0.0"
#endif

Json load_json_from_file(const std::string& path) {
    std::ifstream input_stream(path);
    if (!input_stream) { throw std::runtime_error("Failed to open input JSON: " + path); }

    Json root;
    try {
        input_stream >> root;
    } catch (const std::exception& ex) {
        throw std::runtime_error("Failed to parse JSON in " + path + ": " + std::string(ex.what()));
    }
    return root;
}

OrderedJson to_ordered_json(const Json& value) {
    if (value.is_object()) {
        OrderedJson object = OrderedJson::object();
        for (auto it = value.begin(); it != value.end(); ++it) { object[it.key()] = to_ordered_json(it.value()); }
        return object;
    }
    if (value.is_array()) {
        OrderedJson array = OrderedJson::array();
        for (const auto& item : value) { array.push_back(to_ordered_json(item)); }
        return array;
    }
    return value;
}

OrderedJson build_ordered_config_template(const config::GeneratorMetadata& meta) {
    OrderedJson                     ordered = OrderedJson::object();
    std::unordered_set<std::string> seen;
    seen.reserve(meta.config_params.size());

    for (const auto& param : meta.config_params) {
        if (meta.config_template.contains(param.name)) {
            ordered[param.name] = to_ordered_json(meta.config_template.at(param.name));
            seen.insert(param.name);
        }
    }
    for (auto it = meta.config_template.begin(); it != meta.config_template.end(); ++it) {
        if (!seen.contains(it.key())) { ordered[it.key()] = to_ordered_json(it.value()); }
    }
    return ordered;
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

void print_validation_issues(const std::vector<config::ValidationIssue>& issues, std::ostream& output) {
    for (const auto& issue : issues) {
        output << (issue.warning ? "Validation warning: " : "Validation error: ") << issue.path << " " << issue.message
               << "\n";
    }
}

std::string version_string() {
    return DATAGEN_VERSION;
}

std::string program_display_name() {
    return "datagen";
}

std::string github_url() {
    return "https://github.com/ishizhongqi/datagen";
}

void print_version(std::ostream& output) {
    output << program_display_name() << " " << DATAGEN_VERSION << "\n";
}

}  // namespace datagen::cli
