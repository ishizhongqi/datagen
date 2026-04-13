// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file sql_writer.h

#ifndef DATAGEN_SQL_WRITER_H
#define DATAGEN_SQL_WRITER_H

#include <ostream>
#include <string>
#include <vector>

#include "engine/executor.h"

namespace datagen::output::file {

std::string sql_escape(const std::string& value);

void write_sql_row(
    const std::vector<std::string>& columns,
    const std::vector<bool>&        boolean_columns,
    const engine::Row&              row,
    const std::string&              table_name,
    std::ostream&                   out
);

void write_sql(
    const std::vector<std::string>& columns,
    const std::vector<bool>&        boolean_columns,
    const std::vector<engine::Row>& rows,
    const std::string&              table_name,
    bool                            create_table,
    std::ostream&                   out
);

}  // namespace datagen::output::file

#endif
