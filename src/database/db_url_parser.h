// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file db_url_parser.h

#ifndef DATA_GENERATOR_DB_URL_PARSER_H
#define DATA_GENERATOR_DB_URL_PARSER_H

#include <optional>
#include <string>

#include "database/db_metadata.h"

namespace data_generator::database {

bool parse_db_url(const std::string& url, DbUrl* parsed, std::string* error_message);

std::string build_db_url(
    DbType                   type,
    const std::string&       user,
    const std::string&       password,
    const std::string&       host,
    const std::optional<int>& port,
    const std::string&       database
);

}  // namespace data_generator::database

#endif
