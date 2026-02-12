/// @file test_serialization.cpp

#include <gtest/gtest.h>

#include <sstream>
#include <string>
#include <vector>

#include "core/serialization.h"

using namespace data_generator::core;

namespace {

TEST(SerializationTest, EscapeHelpers) {
    EXPECT_EQ(csv_escape("abc"), "abc");
    EXPECT_EQ(csv_escape("a,b"), "\"a,b\"");
    EXPECT_EQ(csv_escape("a\"b"), "\"a\"\"b\"");

    EXPECT_EQ(sql_escape("abc"), "abc");
    EXPECT_EQ(sql_escape("a'b"), "a''b");
}

TEST(SerializationTest, WriteCsvJsonSql) {
    const std::vector<std::string> columns = {"c1", "c2"};
    const std::vector<Row> rows = {
        Row{std::optional<std::string>("v1"), std::nullopt},
        Row{std::optional<std::string>("a,b"), std::optional<std::string>("x'y")},
    };

    std::ostringstream csv;
    write_csv(columns, rows, csv);
    EXPECT_NE(csv.str().find("c1,c2"), std::string::npos);
    EXPECT_NE(csv.str().find("\"a,b\""), std::string::npos);

    std::ostringstream json;
    write_json(columns, rows, json);
    EXPECT_NE(json.str().find("\"c1\""), std::string::npos);
    EXPECT_NE(json.str().find("null"), std::string::npos);

    std::ostringstream sql_with_create;
    write_sql(columns, rows, "t1", true, sql_with_create);
    EXPECT_NE(sql_with_create.str().find("CREATE TABLE t1"), std::string::npos);
    EXPECT_NE(sql_with_create.str().find("INSERT INTO t1"), std::string::npos);
    EXPECT_NE(sql_with_create.str().find("x''y"), std::string::npos);

    std::ostringstream sql_no_create;
    write_sql(columns, rows, "t1", false, sql_no_create);
    EXPECT_EQ(sql_no_create.str().find("CREATE TABLE"), std::string::npos);
    EXPECT_NE(sql_no_create.str().find("INSERT INTO t1"), std::string::npos);
}

}  // namespace
