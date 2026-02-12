// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file serialization.h

#ifndef DATA_GENERATOR_CORE_SERIALIZATION_H
#define DATA_GENERATOR_CORE_SERIALIZATION_H

#include <optional>
#include <ostream>
#include <string>
#include <vector>

#include "core/configuration.h"

namespace data_generator::core {

using Row = std::vector<std::optional<std::string>>;

std::string csv_escape(const std::string& value);
std::string sql_escape(const std::string& value);

void write_csv(const std::vector<std::string>& columns, const std::vector<Row>& rows, std::ostream& out);

void write_json(const std::vector<std::string>& columns, const std::vector<Row>& rows, std::ostream& out);

void write_sql(
    const std::vector<std::string>& columns,
    const std::vector<Row>&         rows,
    const std::string&              table_name,
    bool                            include_create_table,
    std::ostream&                   out
);

}  // namespace data_generator::core

#endif
