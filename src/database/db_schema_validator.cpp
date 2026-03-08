// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file db_schema_validator.cpp

#include "database/db_schema_validator.h"

#include <algorithm>
#include <cctype>
#include <unordered_map>

namespace data_generator::database {

namespace {

void add_message(
    SchemaValidationReport* report,
    const ValidationLevel   level,
    const std::string&      message
) {
    report->messages.push_back(ValidationMessage{.level = level, .message = message});
}

std::string to_lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

bool is_generator_type_compatible(const std::string& generator_name, const ColumnTypeFamily family) {
    if (generator_name == "integer" || generator_name == "sequence") {
        return family == ColumnTypeFamily::Integer || family == ColumnTypeFamily::Decimal ||
               family == ColumnTypeFamily::Boolean;
    }
    if (generator_name == "decimal") {
        return family == ColumnTypeFamily::Decimal || family == ColumnTypeFamily::Integer;
    }
    if (generator_name == "date") { return family == ColumnTypeFamily::Date || family == ColumnTypeFamily::DateTime; }
    if (generator_name == "time") { return family == ColumnTypeFamily::Time || family == ColumnTypeFamily::DateTime; }
    if (generator_name == "datetime") { return family == ColumnTypeFamily::DateTime || family == ColumnTypeFamily::Date; }
    if (generator_name == "enum_item") {
        return family == ColumnTypeFamily::Enum || family == ColumnTypeFamily::String ||
               family == ColumnTypeFamily::Boolean;
    }
    if (generator_name == "regular_expression") { return true; }
    if (generator_name == "uuid" || generator_name == "text") { return family == ColumnTypeFamily::String || family == ColumnTypeFamily::Unknown; }
    return family == ColumnTypeFamily::String || family == ColumnTypeFamily::Enum || family == ColumnTypeFamily::Unknown;
}

bool has_active_field_null_override(const core::FieldSpec& field) {
    if (!field.raw.contains("null_value") || !field.raw.at("null_value").is_object()) { return false; }

    const auto& null_value = field.raw.at("null_value");
    if (!null_value.value("enabled", false)) { return false; }
    const int percent = null_value.value("percent", 0);
    return percent > 0;
}

}  // namespace

std::size_t SchemaValidationReport::error_count() const {
    return static_cast<std::size_t>(
        std::count_if(messages.begin(), messages.end(), [](const ValidationMessage& message) {
            return message.level == ValidationLevel::Error;
        })
    );
}

std::size_t SchemaValidationReport::warning_count() const {
    return static_cast<std::size_t>(
        std::count_if(messages.begin(), messages.end(), [](const ValidationMessage& message) {
            return message.level == ValidationLevel::Warn;
        })
    );
}

SchemaValidationReport validate_table_schema(const core::GenerationConfig& cfg, const TableMetadata& metadata) {
    SchemaValidationReport report;

    std::unordered_map<std::string, const ColumnMetadata*> column_map;
    column_map.reserve(metadata.columns.size());
    for (const auto& column : metadata.columns) {
        column_map[to_lower(column.name)] = &column;
    }

    for (const auto& field : cfg.fields) {
        const std::string key = to_lower(field.name);
        if (!column_map.contains(key)) {
            add_message(
                &report,
                ValidationLevel::Error,
                "missing target column for field: " + field.name
            );
            continue;
        }

        const ColumnMetadata& column = *column_map.at(key);
        const ColumnTypeFamily family = classify_column_type(column);

        if (!is_generator_type_compatible(field.generator, family)) {
            add_message(
                &report,
                ValidationLevel::Error,
                "field '" + field.name + "' generator '" + field.generator +
                    "' is incompatible with column type '" + column.data_type + "'"
            );
        }

        if (column.default_value.has_value()) {
            add_message(
                &report,
                ValidationLevel::Info,
                "column '" + field.name + "' has default value: " + *column.default_value
            );
        }

        if (column.auto_increment) {
            add_message(
                &report,
                ValidationLevel::Warn,
                "column '" + field.name + "' is AUTO_INCREMENT; generated values may conflict"
            );
        }

        if (column.character_length.has_value()) {
            add_message(
                &report,
                ValidationLevel::Info,
                "column '" + field.name + "' length limit varchar(" +
                    std::to_string(*column.character_length) + ")"
            );
        }

        if (column.numeric_precision.has_value() && column.numeric_scale.has_value()) {
            add_message(
                &report,
                ValidationLevel::Info,
                "column '" + field.name + "' decimal(" +
                    std::to_string(*column.numeric_precision) + "," +
                    std::to_string(*column.numeric_scale) + ")"
            );
        }

        if (column.unsigned_number) {
            add_message(
                &report,
                ValidationLevel::Info,
                "column '" + field.name + "' is unsigned"
            );
        }

        if (!column.enum_values.empty()) {
            add_message(
                &report,
                ValidationLevel::Info,
                "column '" + field.name + "' is enum with " + std::to_string(column.enum_values.size()) +
                    " values"
            );
        }

        if (!column.nullable && has_active_field_null_override(field)) {
            add_message(
                &report,
                ValidationLevel::Error,
                "field '" + field.name + "' null_value is enabled for NOT NULL column"
            );
        }

        if (!column.nullable && cfg.null_policy.configured && cfg.null_policy.null_if_empty) {
            add_message(
                &report,
                ValidationLevel::Warn,
                "column '" + field.name + "' is NOT NULL but null policy may generate NULL"
            );
        }
    }

    if (!metadata.foreign_keys.empty()) {
        add_message(
            &report,
            ValidationLevel::Info,
            "foreign keys detected: " + std::to_string(metadata.foreign_keys.size())
        );
    }

    if (!metadata.triggers.empty()) {
        add_message(
            &report,
            ValidationLevel::Warn,
            "triggers detected (import will continue): " + std::to_string(metadata.triggers.size())
        );
    }

    if (metadata.partitioned) {
        add_message(
            &report,
            ValidationLevel::Info,
            "table is partitioned"
        );
    }

    if (metadata.indexes.size() > 8) {
        add_message(
            &report,
            ValidationLevel::Warn,
            "index count is high (" + std::to_string(metadata.indexes.size()) + "), write throughput may drop"
        );
    } else {
        add_message(
            &report,
            ValidationLevel::Info,
            "index count: " + std::to_string(metadata.indexes.size())
        );
    }

    return report;
}

std::string validation_level_to_string(const ValidationLevel level) {
    switch (level) {
    case ValidationLevel::Info : return "INFO";
    case ValidationLevel::Warn : return "WARN";
    case ValidationLevel::Error: return "ERROR";
    }
    return "INFO";
}

}  // namespace data_generator::database
