// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file csv_writer.h

#ifndef DATA_GENERATOR_CSV_WRITER_H
#define DATA_GENERATOR_CSV_WRITER_H

#include <ostream>
#include <string>
#include <vector>

#include "config/configuration.h"
#include "engine/executor.h"

namespace data_generator::output::file {

struct DelimitedWriterOptions {
    std::string        delimiter = ",";
    std::string        quote = "\"";
    bool               header = true;
    config::LineEnding line_ending = config::LineEnding::LF;
};

std::string escape_delimited_value(const std::string& value, const DelimitedWriterOptions& options);

void write_delimited_header(
    const std::vector<std::string>& columns,
    std::ostream&                   out,
    const DelimitedWriterOptions&   options
);
void write_delimited_row(
    const engine::Row&            row,
    std::ostream&                 out,
    const DelimitedWriterOptions& options
);
void write_delimited(
    const std::vector<std::string>& columns,
    const std::vector<engine::Row>& rows,
    std::ostream&                   out,
    const DelimitedWriterOptions&   options
);

}  // namespace data_generator::output::file

#endif
