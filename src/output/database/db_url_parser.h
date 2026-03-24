// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file db_url_parser.h

#ifndef DATA_GENERATOR_DB_URL_PARSER_H
#define DATA_GENERATOR_DB_URL_PARSER_H

#include <string>

#include "output/database/db_metadata.h"

namespace data_generator::database {

bool parse_db_connection(const std::string& connection, DbUrl* parsed, std::string* error_message);

}  // namespace data_generator::database

#endif
