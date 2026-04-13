/// @file test_db_schema_validator.cpp

#include <gtest/gtest.h>

#include <algorithm>
#include <string>

#include "config/configuration.h"
#include "output/database/db_schema_validator.h"

using datagen::config::FieldSpec;
using datagen::config::GenerationConfig;
using datagen::database::ColumnMetadata;
using datagen::database::TableMetadata;
using datagen::database::ValidationLevel;
using datagen::database::validate_table_schema;

namespace {

FieldSpec make_field(const std::string& name, const std::string& generator) {
    FieldSpec field;
    field.name = name;
    field.generator = generator;
    field.raw = datagen::config::Json::object();
    return field;
}

bool has_message(const std::vector<datagen::database::ValidationMessage>& messages,
                 const std::string& needle) {
    return std::any_of(messages.begin(), messages.end(), [&](const auto& message) {
        return message.message.find(needle) != std::string::npos;
    });
}

}  // namespace

TEST(DbSchemaValidatorTest, ReportsErrorsWarningsAndInfo) {
    GenerationConfig cfg;
    cfg.fields.push_back(make_field("id", "integer"));
    cfg.fields.push_back(make_field("status", "enum_item"));
    cfg.fields.push_back(make_field("price", "text"));
    cfg.fields.push_back(make_field("missing", "text"));

    TableMetadata metadata;
    metadata.table_name = "t_data";

    ColumnMetadata id;
    id.name = "id";
    id.data_type = "int";
    id.default_value = "0";
    id.auto_increment = true;
    id.unsigned_number = true;
    metadata.columns.push_back(id);

    ColumnMetadata status;
    status.name = "status";
    status.data_type = "enum";
    status.enum_values = {"A", "B"};
    metadata.columns.push_back(status);

    ColumnMetadata price;
    price.name = "price";
    price.data_type = "decimal";
    price.numeric_precision = 10;
    price.numeric_scale = 2;
    metadata.columns.push_back(price);

    ColumnMetadata short_name;
    short_name.name = "short_name";
    short_name.data_type = "varchar";
    short_name.character_length = 5;
    metadata.columns.push_back(short_name);

    metadata.foreign_keys.push_back({"id", "other", "id"});
    metadata.triggers.push_back({"trg", "insert"});
    metadata.partitioned = true;

    for (int i = 0; i < 9; ++i) {
        metadata.indexes.push_back({"idx" + std::to_string(i), false, {"id"}});
    }

    const auto report = validate_table_schema(cfg, metadata);

    EXPECT_GT(report.error_count(), 0u);
    EXPECT_GT(report.warning_count(), 0u);

    EXPECT_TRUE(has_message(report.messages, "missing target column"));
    EXPECT_TRUE(has_message(report.messages, "incompatible"));
    EXPECT_TRUE(has_message(report.messages, "AUTO_INCREMENT"));
    EXPECT_TRUE(has_message(report.messages, "foreign keys detected"));
    EXPECT_TRUE(has_message(report.messages, "index count"));
}

TEST(DbSchemaValidatorTest, ValidationLevelToStringCoversAllBranches) {
    EXPECT_EQ(datagen::database::validation_level_to_string(ValidationLevel::Info), "INFO");
    EXPECT_EQ(datagen::database::validation_level_to_string(ValidationLevel::Warn), "WARN");
    EXPECT_EQ(datagen::database::validation_level_to_string(ValidationLevel::Error), "ERROR");
    EXPECT_EQ(
        datagen::database::validation_level_to_string(static_cast<ValidationLevel>(99)),
        "INFO"
    );
}

TEST(DbSchemaValidatorTest, CoversCharacterLengthAndCompatibilityFallbackBranches) {
    GenerationConfig cfg;
    cfg.fields.push_back(make_field("short_name", "text"));
    cfg.fields.push_back(make_field("amount", "decimal"));
    cfg.fields.push_back(make_field("pattern", "regular_expression"));
    cfg.fields.push_back(make_field("email", "email"));

    TableMetadata metadata;
    metadata.table_name = "t_data";

    ColumnMetadata short_name;
    short_name.name = "short_name";
    short_name.data_type = "varchar";
    short_name.character_length = 12;
    metadata.columns.push_back(short_name);

    ColumnMetadata amount;
    amount.name = "amount";
    amount.data_type = "integer";
    metadata.columns.push_back(amount);

    ColumnMetadata pattern;
    pattern.name = "pattern";
    pattern.data_type = "text";
    metadata.columns.push_back(pattern);

    ColumnMetadata email;
    email.name = "email";
    email.data_type = "varchar";
    metadata.columns.push_back(email);

    const auto report = validate_table_schema(cfg, metadata);

    EXPECT_EQ(report.error_count(), 0u);
    EXPECT_TRUE(has_message(report.messages, "length limit varchar(12)"));
    EXPECT_TRUE(has_message(report.messages, "index count: 0"));
}

TEST(DbSchemaValidatorTest, BooleanGeneratorSupportsBooleanAndNumericColumns) {
    GenerationConfig cfg;
    cfg.fields.push_back(make_field("is_active", "boolean"));
    cfg.fields.push_back(make_field("legacy_flag", "boolean"));

    TableMetadata metadata;
    metadata.table_name = "t_data";

    ColumnMetadata boolean_column;
    boolean_column.name = "is_active";
    boolean_column.data_type = "boolean";
    metadata.columns.push_back(boolean_column);

    ColumnMetadata numeric_column;
    numeric_column.name = "legacy_flag";
    numeric_column.data_type = "number";
    numeric_column.numeric_precision = 1;
    numeric_column.numeric_scale = 0;
    metadata.columns.push_back(numeric_column);

    const auto report = validate_table_schema(cfg, metadata);
    EXPECT_EQ(report.error_count(), 0u);
}
