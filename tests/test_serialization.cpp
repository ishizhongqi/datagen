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
    EXPECT_EQ(output::file::csv_escape("abc"), "abc");
    EXPECT_EQ(output::file::csv_escape("a,b"), "\"a,b\"");
    EXPECT_EQ(output::file::csv_escape("a\"b"), "\"a\"\"b\"");

    EXPECT_EQ(output::file::sql_escape("abc"), "abc");
    EXPECT_EQ(output::file::sql_escape("a'b"), "a''b");
}

TEST(SerializationTest, WriteCsvJsonSql) {
    const std::vector<std::string> columns = {"c1", "c2"};
    const std::vector<engine::Row> rows = {
        engine::Row{std::optional<std::string>("v1"), std::nullopt},
        engine::Row{std::optional<std::string>("a,b"), std::optional<std::string>("x'y")},
    };

    std::ostringstream csv;
    output::file::write_csv(columns, rows, csv);
    EXPECT_NE(csv.str().find("c1,c2"), std::string::npos);
    EXPECT_NE(csv.str().find("\"a,b\""), std::string::npos);

    std::ostringstream json;
    output::file::write_json(columns, rows, json);
    EXPECT_NE(json.str().find("\"c1\""), std::string::npos);
    EXPECT_NE(json.str().find("null"), std::string::npos);

    std::ostringstream sql_out;
    output::file::write_sql(columns, rows, "t1", sql_out);
    EXPECT_EQ(sql_out.str().find("CREATE TABLE"), std::string::npos);
    EXPECT_NE(sql_out.str().find("INSERT INTO t1"), std::string::npos);
    EXPECT_NE(sql_out.str().find("x''y"), std::string::npos);
}

}  // namespace
