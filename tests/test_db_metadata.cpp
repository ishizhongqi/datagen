/// @file test_db_metadata.cpp

#include <gtest/gtest.h>

#include "output/database/db_metadata.h"

using datagen::database::ColumnMetadata;
using datagen::database::ColumnTypeFamily;
using datagen::database::DbType;
using datagen::database::classify_column_type;
using datagen::database::db_type_to_string;

TEST(DbMetadataTest, TypeToString) {
    EXPECT_EQ(db_type_to_string(DbType::Mysql), "mysql");
    EXPECT_EQ(db_type_to_string(DbType::Postgresql), "postgresql");
    EXPECT_EQ(db_type_to_string(DbType::Oracle), "oracle");
    EXPECT_EQ(db_type_to_string(DbType::Sqlite), "sqlite");
    EXPECT_EQ(db_type_to_string(DbType::Unknown), "unknown");
}

TEST(DbMetadataTest, ClassifyColumnTypes) {
    ColumnMetadata column;

    column.data_type = "tinyint(1)";
    EXPECT_EQ(classify_column_type(column), ColumnTypeFamily::Boolean);

    column.data_type = "int";
    EXPECT_EQ(classify_column_type(column), ColumnTypeFamily::Integer);

    column.data_type = "decimal(10,2)";
    EXPECT_EQ(classify_column_type(column), ColumnTypeFamily::Decimal);

    column.data_type = "timestamp";
    EXPECT_EQ(classify_column_type(column), ColumnTypeFamily::DateTime);

    column.data_type = "date";
    EXPECT_EQ(classify_column_type(column), ColumnTypeFamily::Date);

    column.data_type = "time";
    EXPECT_EQ(classify_column_type(column), ColumnTypeFamily::Time);

    column.data_type = "enum('A','B')";
    EXPECT_EQ(classify_column_type(column), ColumnTypeFamily::Enum);

    column.data_type = "blob";
    EXPECT_EQ(classify_column_type(column), ColumnTypeFamily::Binary);

    column.data_type = "varchar(255)";
    EXPECT_EQ(classify_column_type(column), ColumnTypeFamily::String);

    column.data_type = "custom";
    EXPECT_EQ(classify_column_type(column), ColumnTypeFamily::Unknown);
}
