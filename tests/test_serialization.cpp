/// @file test_serialization.cpp

#include <gtest/gtest.h>

#include <sstream>
#include <string>
#include <vector>

#include "engine/executor.h"
#include "output/file/csv_writer.h"
#include "output/file/json_writer.h"
#include "output/file/sql_writer.h"

using namespace data_generator;

namespace {

TEST(SerializationTest, EscapeHelpers) {
    output::file::DelimitedWriterOptions options;
    EXPECT_EQ(output::file::escape_delimited_value("abc", options), "abc");
    EXPECT_EQ(output::file::escape_delimited_value("a,b", options), "\"a,b\"");
    EXPECT_EQ(output::file::escape_delimited_value("a\"b", options), "\"a\"\"b\"");

    EXPECT_EQ(output::file::sql_escape("abc"), "abc");
    EXPECT_EQ(output::file::sql_escape("a'b"), "a''b");
}

TEST(SerializationTest, WriteCsvJsonSql) {
    const std::vector<std::string> columns = {"c1", "c2"};
    const std::vector<bool> boolean_columns = {false, true};
    const std::vector<engine::Row> rows = {
        engine::Row{std::optional<std::string>("v1"), std::nullopt},
        engine::Row{std::optional<std::string>("a,b"), std::optional<std::string>("true")},
    };

    std::ostringstream csv;
    output::file::DelimitedWriterOptions options;
    output::file::write_delimited(columns, rows, csv, options);
    EXPECT_NE(csv.str().find("c1,c2"), std::string::npos);
    EXPECT_NE(csv.str().find("\"a,b\""), std::string::npos);

    std::ostringstream json;
    config::JsonOptions json_options;
    json_options.array = true;
    json_options.include_null = true;
    output::file::write_json(columns, boolean_columns, rows, json, json_options);
    EXPECT_NE(json.str().find("\"c1\""), std::string::npos);
    EXPECT_NE(json.str().find("null"), std::string::npos);
    EXPECT_NE(json.str().find("true"), std::string::npos);

    std::ostringstream sql_out;
    output::file::write_sql(columns, boolean_columns, rows, "t1", false, sql_out);
    EXPECT_EQ(sql_out.str().find("CREATE TABLE"), std::string::npos);
    EXPECT_NE(sql_out.str().find("INSERT INTO t1"), std::string::npos);
    EXPECT_NE(sql_out.str().find("TRUE"), std::string::npos);
}

}  // namespace
