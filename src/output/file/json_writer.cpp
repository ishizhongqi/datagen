// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file json_writer.cpp

#include "output/file/json_writer.h"

#include <algorithm>
#include <optional>
#include <nlohmann/json.hpp>
#include <cctype>

namespace data_generator::output::file {

namespace {

std::optional<bool> parse_boolean_token(std::string raw) {
    std::transform(raw.begin(), raw.end(), raw.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    if (raw == "true" || raw == "1") { return true; }
    if (raw == "false" || raw == "0") { return false; }
    return std::nullopt;
}

nlohmann::json
    build_json_object(
        const std::vector<std::string>& columns,
        const std::vector<bool>&        boolean_columns,
        const engine::Row&              row,
        const bool                      include_null
    ) {
    nlohmann::json entry = nlohmann::json::object();
    for (size_t i = 0; i < columns.size(); ++i) {
        if (i >= row.size() || !row[i].has_value()) {
            if (include_null) { entry[columns[i]] = nullptr; }
        } else {
            if (i < boolean_columns.size() && boolean_columns[i]) {
                if (const auto bool_value = parse_boolean_token(*row[i]); bool_value.has_value()) {
                    entry[columns[i]] = *bool_value;
                    continue;
                }
            }
            entry[columns[i]] = *row[i];
        }
    }
    return entry;
}

}  // namespace

void write_json_array_start(std::ostream& out) {
    out << "[\n";
}

void write_json_row(
    const std::vector<std::string>& columns,
    const std::vector<bool>&        boolean_columns,
    const engine::Row&              row,
    std::ostream&                   out,
    const bool                      first,
    const config::JsonOptions&      options
) {
    nlohmann::json entry = build_json_object(columns, boolean_columns, row, options.include_null);
    if (options.array) {
        if (!first) { out << ",\n"; }
        out << "  " << entry.dump();
    } else {
        out << entry.dump() << "\n";
    }
}

void write_json_array_end(std::ostream& out) {
    out << "\n]\n";
}

void write_json(
    const std::vector<std::string>& columns,
    const std::vector<bool>&        boolean_columns,
    const std::vector<engine::Row>& rows,
    std::ostream&                   out,
    const config::JsonOptions&      options
) {
    if (options.array) {
        nlohmann::json result = nlohmann::json::array();
        result.get_ref<nlohmann::json::array_t&>().reserve(rows.size());

        for (const auto& row : rows) {
            result.push_back(build_json_object(columns, boolean_columns, row, options.include_null));
        }

        out << result.dump(2) << "\n";
        return;
    }

    for (const auto& row : rows) {
        out << build_json_object(columns, boolean_columns, row, options.include_null).dump() << "\n";
    }
}

}  // namespace data_generator::output::file
