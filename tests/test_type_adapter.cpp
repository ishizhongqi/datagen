/// @file test_type_adapter.cpp

#include <gtest/gtest.h>

#include <optional>

#include "output/database/type_adapter.h"

using data_generator::database::ColumnMetadata;
using data_generator::database::DefaultTypeAdapter;

namespace {

ColumnMetadata make_column(const std::string& name, const std::string& type) {
    ColumnMetadata column;
    column.name = name;
    column.data_type = type;
    return column;
}

}  // namespace

TEST(TypeAdapterTest, AdaptsIntegerValues) {
    DefaultTypeAdapter adapter;
    ColumnMetadata column = make_column("id", "int");

    auto ok = adapter.adapt(column, std::optional<std::string>("42"));
    EXPECT_TRUE(ok.ok);
    EXPECT_EQ(ok.sql_literal, "42");

    auto bad = adapter.adapt(column, std::optional<std::string>("not_int"));
    EXPECT_FALSE(bad.ok);
    EXPECT_EQ(bad.error_type, "type_conversion");

    column.unsigned_number = true;
    auto neg = adapter.adapt(column, std::optional<std::string>("-3"));
    EXPECT_FALSE(neg.ok);
}

TEST(TypeAdapterTest, AdaptsDecimalAndBooleanValues) {
    DefaultTypeAdapter adapter;

    ColumnMetadata decimal = make_column("amount", "decimal");
    auto ok = adapter.adapt(decimal, std::optional<std::string>("12.50"));
    EXPECT_TRUE(ok.ok);
    EXPECT_EQ(ok.sql_literal, "12.50");

    auto bad = adapter.adapt(decimal, std::optional<std::string>("12.1.3"));
    EXPECT_FALSE(bad.ok);

    ColumnMetadata boolean = make_column("flag", "boolean");
    auto yes = adapter.adapt(boolean, std::optional<std::string>("YES"));
    EXPECT_TRUE(yes.ok);
    EXPECT_EQ(yes.sql_literal, "1");

    auto invalid = adapter.adapt(boolean, std::optional<std::string>("maybe"));
    EXPECT_FALSE(invalid.ok);
}

TEST(TypeAdapterTest, AdaptsTemporalEnumAndStringValues) {
    DefaultTypeAdapter adapter;

    ColumnMetadata date = make_column("dt", "date");
    auto ok_date = adapter.adapt(date, std::optional<std::string>("2024-01-01"));
    EXPECT_TRUE(ok_date.ok);
    EXPECT_EQ(ok_date.sql_literal, "'2024-01-01'");

    ColumnMetadata time = make_column("tm", "time");
    auto ok_time = adapter.adapt(time, std::optional<std::string>("12:30:00"));
    EXPECT_TRUE(ok_time.ok);

    ColumnMetadata enumeration = make_column("status", "enum");
    enumeration.enum_values = {"A", "B"};
    auto ok_enum = adapter.adapt(enumeration, std::optional<std::string>("A"));
    EXPECT_TRUE(ok_enum.ok);

    auto bad_enum = adapter.adapt(enumeration, std::optional<std::string>("Z"));
    EXPECT_FALSE(bad_enum.ok);

    ColumnMetadata text = make_column("name", "varchar");
    text.character_length = 3;
    auto ok_text = adapter.adapt(text, std::optional<std::string>("abc"));
    EXPECT_TRUE(ok_text.ok);

    auto too_long = adapter.adapt(text, std::optional<std::string>("abcd"));
    EXPECT_FALSE(too_long.ok);
}

TEST(TypeAdapterTest, AdaptsNullValues) {
    DefaultTypeAdapter adapter;
    ColumnMetadata column = make_column("value", "text");

    auto result = adapter.adapt(column, std::nullopt);
    EXPECT_TRUE(result.ok);
    EXPECT_TRUE(result.is_null);
    EXPECT_EQ(result.sql_literal, "NULL");
}
