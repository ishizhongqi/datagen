// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file linkage_helper.h

#ifndef DATA_GENERATOR_LINKAGE_HELPER_H
#define DATA_GENERATOR_LINKAGE_HELPER_H

#include <nlohmann/json.hpp>

#include <optional>
#include <string>

namespace data_generator::generator {

using Json = nlohmann::json;

std::optional<std::string> parse_linkage_key(const Json& column);

}  // namespace data_generator::generator

#endif  // DATA_GENERATOR_LINKAGE_HELPER_H
