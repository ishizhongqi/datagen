// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file json_writer.h

#ifndef DATA_GENERATOR_JSON_WRITER_H
#define DATA_GENERATOR_JSON_WRITER_H

#include <ostream>
#include <string>
#include <vector>

#include "config/configuration.h"
#include "engine/executor.h"

namespace data_generator::output::file {

void write_json_array_start(std::ostream& out);
void write_json_row(
    const std::vector<std::string>& columns,
    const engine::Row&              row,
    std::ostream&                   out,
    bool                            first,
    const config::JsonOptions&      options
);
void write_json_array_end(std::ostream& out);

void write_json(
    const std::vector<std::string>& columns,
    const std::vector<engine::Row>& rows,
    std::ostream&                   out,
    const config::JsonOptions&      options
);

}  // namespace data_generator::output::file

#endif
