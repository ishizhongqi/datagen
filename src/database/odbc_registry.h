// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file odbc_registry.h

#ifndef DATA_GENERATOR_ODBC_REGISTRY_H
#define DATA_GENERATOR_ODBC_REGISTRY_H

#include <string>
#include <vector>

namespace data_generator::database {

struct OdbcDriverInfo {
    std::string name;
    std::string attributes;
};

bool list_odbc_drivers(std::vector<OdbcDriverInfo>* drivers, std::string* error_message);

}  // namespace data_generator::database

#endif
