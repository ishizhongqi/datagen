// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

#ifndef DATA_GENERATOR_DATABASE_DB_SCHEMA_VALIDATOR_H
#define DATA_GENERATOR_DATABASE_DB_SCHEMA_VALIDATOR_H

#include <string>
#include <vector>

#include "core/configuration.h"
#include "database/db_metadata.h"

namespace data_generator::database {

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

    std::size_t error_count() const;
    std::size_t warning_count() const;
};

SchemaValidationReport validate_table_schema(
    const core::GenerationConfig& cfg,
    const TableMetadata&          metadata
);

std::string validation_level_to_string(ValidationLevel level);

}  // namespace data_generator::database

#endif
