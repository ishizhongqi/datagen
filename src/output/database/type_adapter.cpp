// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file type_adapter.cpp

#include "output/database/type_adapter.h"

#include <algorithm>
#include <cctype>
#include <charconv>

namespace datagen::database {

namespace {

std::string to_lower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return text;
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

AdaptedValue make_error(std::string error_type, std::string error_message) {
    AdaptedValue value;
    value.ok            = false;
    value.error_type    = std::move(error_type);
    value.error_message = std::move(error_message);
    return value;
}

AdaptedValue make_success_value(const std::string& converted, const std::string& literal) {
    AdaptedValue value;
    value.ok              = true;
    value.converted_value = converted;
    value.sql_literal     = literal;
    return value;
}

std::optional<bool> parse_boolean_token(const std::string& raw) {
    const std::string lower = to_lower(raw);
    if (lower == "1" || lower == "true" || lower == "t" || lower == "yes") { return true; }
    if (lower == "0" || lower == "false" || lower == "f" || lower == "no") { return false; }
    return std::nullopt;
}

}  // namespace

AdaptedValue
    DefaultTypeAdapter::adapt(const ColumnMetadata& column, const std::optional<std::string>& raw_value) const {
    if (!raw_value.has_value()) {
        AdaptedValue value;
        value.ok              = true;
        value.is_null         = true;
        value.converted_value = "NULL";
        value.sql_literal     = "NULL";
        return value;
    }

    const std::string&     raw    = *raw_value;
    const ColumnTypeFamily family = classify_column_type(column);

    switch (family) {
    case ColumnTypeFamily::Integer: {
        if (const auto bool_value = parse_boolean_token(raw); bool_value.has_value()) {
            return make_success_value(*bool_value ? "1" : "0", *bool_value ? "1" : "0");
        }
        if (raw.empty()) { return make_error("type_conversion", "empty string cannot be converted to integer"); }
        long long parsed     = 0;
        const auto [ptr, ec] = std::from_chars(raw.data(), raw.data() + raw.size(), parsed);
        if (ec != std::errc() || ptr != raw.data() + raw.size()) {
            return make_error("type_conversion", "invalid integer: " + raw);
        }
        if (column.unsigned_number && parsed < 0) {
            return make_error("type_conversion", "negative value for unsigned integer: " + raw);
        }
        return make_success_value(std::to_string(parsed), std::to_string(parsed));
    }
    case ColumnTypeFamily::Decimal: {
        if (const auto bool_value = parse_boolean_token(raw); bool_value.has_value()) {
            return make_success_value(*bool_value ? "1" : "0", *bool_value ? "1" : "0");
        }
        if (raw.empty()) { return make_error("type_conversion", "empty string cannot be converted to decimal"); }
        bool decimal_point_found = false;
        for (const char ch : raw) {
            if (ch == '.') {
                if (decimal_point_found) { return make_error("type_conversion", "invalid decimal: " + raw); }
                decimal_point_found = true;
                continue;
            }
            if (ch == '-' || ch == '+') { continue; }
            if (!std::isdigit(static_cast<unsigned char>(ch))) {
                return make_error("type_conversion", "invalid decimal: " + raw);
            }
        }
        return make_success_value(raw, raw);
    }
    case ColumnTypeFamily::Boolean: {
        const auto bool_value = parse_boolean_token(raw);
        if (!bool_value.has_value()) { return make_error("type_conversion", "invalid bool: " + raw); }
        const std::string lower_type = to_lower(column.data_type);
        if (lower_type == "boolean" || lower_type == "bool") {
            return make_success_value(*bool_value ? "true" : "false", *bool_value ? "TRUE" : "FALSE");
        }
        return make_success_value(*bool_value ? "1" : "0", *bool_value ? "1" : "0");
    }
    case ColumnTypeFamily::Date:
    case ColumnTypeFamily::Time:
    case ColumnTypeFamily::DateTime: {
        if (raw.empty()) {
            return make_error("type_conversion", "empty string cannot be converted to datetime/date/time");
        }
        // Keep normalized string pass-through for now; database validates strict format.
        return make_success_value(raw, "'" + sql_escape(raw) + "'");
    }
    case ColumnTypeFamily::Enum: {
        if (!column.enum_values.empty()) {
            const bool matched =
                std::find(column.enum_values.begin(), column.enum_values.end(), raw) != column.enum_values.end();
            if (!matched) { return make_error("type_conversion", "enum value out of range: " + raw); }
        }
        return make_success_value(raw, "'" + sql_escape(raw) + "'");
    }
    case ColumnTypeFamily::String:
    case ColumnTypeFamily::Binary:
    case ColumnTypeFamily::Unknown: {
        if (column.character_length.has_value() &&
            *column.character_length > 0 &&
            static_cast<int>(raw.size()) > *column.character_length) {
            return make_error(
                "type_conversion",
                "value length exceeds column limit " + std::to_string(*column.character_length)
            );
        }
        return make_success_value(raw, "'" + sql_escape(raw) + "'");
    }
    }

    return make_error("type_conversion", "unhandled type conversion");
}

}  // namespace datagen::database
