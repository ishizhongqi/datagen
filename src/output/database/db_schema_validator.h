// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file db_schema_validator.h

#ifndef DATAGEN_DB_SCHEMA_VALIDATOR_H
#define DATAGEN_DB_SCHEMA_VALIDATOR_H

#include <string>
#include <vector>

#include "config/configuration.h"
#include "output/database/db_metadata.h"

namespace datagen::database {

enum class ValidationLevel {
    Info,
    Warn,
    Error,
};

struct ValidationMessage {
    ValidationLevel level = ValidationLevel::Info;
    std::string     message;
};

struct SchemaValidationReport {
    std::vector<ValidationMessage> messages;

    [[nodiscard]] std::size_t error_count() const;
    [[nodiscard]] std::size_t warning_count() const;
};

SchemaValidationReport validate_table_schema(const config::GenerationConfig& cfg, const TableMetadata& metadata);

std::string validation_level_to_string(ValidationLevel level);

}  // namespace datagen::database

#endif
