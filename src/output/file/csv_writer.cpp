// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file csv_writer.cpp

#include "output/file/csv_writer.h"

namespace data_generator::output::file {

namespace {

std::string line_ending_text(const config::LineEnding ending) {
    return ending == config::LineEnding::CRLF ? "\r\n" : "\n";
}

bool needs_quoting(const std::string& value, const DelimitedWriterOptions& options) {
    if (options.quote.empty()) { return false; }
    if (value.find(options.delimiter) != std::string::npos) { return true; }
    if (value.find(options.quote) != std::string::npos) { return true; }
    if (value.find('\n') != std::string::npos || value.find('\r') != std::string::npos) { return true; }
    return false;
}

}  // namespace

std::string escape_delimited_value(const std::string& value, const DelimitedWriterOptions& options) {
    if (!needs_quoting(value, options)) { return value; }
    std::string escaped;
    escaped.reserve(value.size() + options.quote.size() * 2);
    escaped += options.quote;
    std::size_t pos = 0;
    while (pos < value.size()) {
        if (!options.quote.empty() &&
            value.compare(pos, options.quote.size(), options.quote) == 0) {
            escaped += options.quote;
            escaped += options.quote;
            pos += options.quote.size();
        } else {
            escaped.push_back(value[pos]);
            ++pos;
        }
    }
    escaped += options.quote;
    return escaped;
}

void write_delimited_header(
    const std::vector<std::string>& columns,
    std::ostream&                   out,
    const DelimitedWriterOptions&   options
) {
    if (!options.header) { return; }
    for (size_t i = 0; i < columns.size(); ++i) {
        if (i > 0) { out << options.delimiter; }
        out << escape_delimited_value(columns[i], options);
    }
    out << line_ending_text(options.line_ending);
}

void write_delimited_row(
    const engine::Row&            row,
    std::ostream&                 out,
    const DelimitedWriterOptions& options
) {
    for (size_t i = 0; i < row.size(); ++i) {
        if (i > 0) { out << options.delimiter; }
        out << escape_delimited_value(row[i].value_or(""), options);
    }
    out << line_ending_text(options.line_ending);
}

void write_delimited(
    const std::vector<std::string>& columns,
    const std::vector<engine::Row>& rows,
    std::ostream&                   out,
    const DelimitedWriterOptions&   options
) {
    write_delimited_header(columns, out, options);
    for (const auto& row : rows) {
        write_delimited_row(row, out, options);
    }
}

}  // namespace data_generator::output::file
