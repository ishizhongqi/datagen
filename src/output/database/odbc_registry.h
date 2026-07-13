// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file odbc_registry.h

#ifndef DATAGEN_ODBC_REGISTRY_H
#define DATAGEN_ODBC_REGISTRY_H

#include <string>
#include <vector>

namespace datagen::database {

struct OdbcDriverInfo {
    std::string name;
    std::string attributes;
};

bool list_odbc_drivers(std::vector<OdbcDriverInfo>* drivers, std::string* error_message);

}  // namespace datagen::database

#endif
