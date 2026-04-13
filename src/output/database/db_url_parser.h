// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file db_url_parser.h

#ifndef DATAGEN_DB_URL_PARSER_H
#define DATAGEN_DB_URL_PARSER_H

#include <string>

#include "output/database/db_metadata.h"

namespace datagen::database {

bool parse_db_connection(const std::string& connection, DbUrl* parsed, std::string* error_message);

}  // namespace datagen::database

#endif
