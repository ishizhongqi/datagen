/// @file test_file_writers.cpp

#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include "config/configuration.h"
#include "engine/executor.h"
#include "output/file/csv_writer.h"
#include "output/file/json_writer.h"
#include "output/file/sql_writer.h"

using data_generator::config::JsonOptions;
using data_generator::config::LineEnding;
using data_generator::engine::Row;
using data_generator::output::file::DelimitedWriterOptions;
using data_generator::output::file::escape_delimited_value;
using data_generator::output::file::sql_escape;
using data_generator::output::file::write_delimited;
using data_generator::output::file::write_delimited_header;
using data_generator::output::file::write_delimited_row;
using data_generator::output::file::write_json;
using data_generator::output::file::write_json_array_end;
using data_generator::output::file::write_json_array_start;
using data_generator::output::file::write_json_row;
using data_generator::output::file::write_sql;
using data_generator::output::file::write_sql_row;

TEST(FileWriterTest, CsvEscapesAndLineEndings) {
    const std::vector<std::string> columns = {"name", "note"};
    Row row;
    row.emplace_back("a,b");
    row.emplace_back("c\"d");

    DelimitedWriterOptions options;
    options.delimiter = ",";
    options.quote = "\"";
    options.header = true;
    options.line_ending = LineEnding::CRLF;

    std::ostringstream out;
    write_delimited_header(columns, out, options);
    write_delimited_row(row, out, options);

    EXPECT_EQ(out.str(), "name,note\r\n\"a,b\",\"c\"\"d\"\r\n");
}

TEST(FileWriterTest, CsvNoQuoteLeavesRawValue) {
    DelimitedWriterOptions options;
    options.delimiter = ",";
    options.quote.clear();
    options.header = false;
    options.line_ending = LineEnding::LF;

    EXPECT_EQ(escape_delimited_value("a,b", options), "a,b");
}

TEST(FileWriterTest, WriteDelimitedCollection) {
    const std::vector<std::string> columns = {"id"};
    std::vector<Row> rows;
    Row row;
    row.emplace_back(std::nullopt);
    rows.push_back(row);

    DelimitedWriterOptions options;
    options.delimiter = ";";
    options.quote = "\"";
    options.header = true;
    options.line_ending = LineEnding::LF;

    std::ostringstream out;
    write_delimited(columns, rows, out, options);

    EXPECT_EQ(out.str(), "id\n\n");
}

TEST(FileWriterTest, JsonArrayAndLineOutput) {
    const std::vector<std::string> columns = {"id", "note"};
    const std::vector<bool> boolean_columns = {false, true};
    Row row1;
    row1.emplace_back("1");
    row1.emplace_back(std::nullopt);
    Row row2;
    row2.emplace_back("2");
    row2.emplace_back("ok");

    JsonOptions array_options;
    array_options.array = true;
    array_options.include_null = false;

    std::ostringstream array_out;
    write_json_array_start(array_out);
    write_json_row(columns, boolean_columns, row1, array_out, true, array_options);
    write_json_row(columns, boolean_columns, row2, array_out, false, array_options);
    write_json_array_end(array_out);

    const auto parsed_array = nlohmann::json::parse(array_out.str());
    ASSERT_TRUE(parsed_array.is_array());
    ASSERT_EQ(parsed_array.size(), 2u);
    EXPECT_EQ(parsed_array[0]["id"], "1");
    EXPECT_FALSE(parsed_array[0].contains("note"));

    JsonOptions line_options;
    line_options.array = false;
    line_options.include_null = true;

    std::ostringstream line_out;
    write_json_row(columns, boolean_columns, row1, line_out, true, line_options);
    write_json_row(columns, boolean_columns, row2, line_out, false, line_options);

    std::istringstream line_in(line_out.str());
    std::string line;
    ASSERT_TRUE(std::getline(line_in, line));
    const auto first = nlohmann::json::parse(line);
    EXPECT_TRUE(first.contains("note"));
    EXPECT_TRUE(first["note"].is_null());
}

TEST(FileWriterTest, WriteJsonCollection) {
    const std::vector<std::string> columns = {"id"};
    const std::vector<bool> boolean_columns = {true};
    std::vector<Row> rows;
    Row row;
    row.emplace_back("true");
    rows.push_back(row);

    JsonOptions options;
    options.array = true;
    options.include_null = true;

    std::ostringstream out;
    write_json(columns, boolean_columns, rows, out, options);

    const auto parsed = nlohmann::json::parse(out.str());
    ASSERT_TRUE(parsed.is_array());
    EXPECT_TRUE(parsed[0]["id"].get<bool>());
}

TEST(FileWriterTest, SqlWriterEscapesAndCreatesTable) {
    EXPECT_EQ(sql_escape("O'Reilly"), "O''Reilly");

    const std::vector<std::string> columns = {"id", "note"};
    const std::vector<bool> boolean_columns = {true, false};
    std::vector<Row> rows;
    Row row;
    row.emplace_back("false");
    row.emplace_back(std::nullopt);
    rows.push_back(row);

    std::ostringstream out;
    write_sql(columns, boolean_columns, rows, "t_data", true, out);

    const std::string sql = out.str();
    EXPECT_NE(sql.find("CREATE TABLE IF NOT EXISTS t_data"), std::string::npos);
    EXPECT_NE(sql.find("id BOOLEAN"), std::string::npos);

    std::ostringstream row_out;
    write_sql_row(columns, boolean_columns, row, "t_data", row_out);
    EXPECT_EQ(row_out.str(), "INSERT INTO t_data (id, note) VALUES (FALSE, NULL);\n");
}
