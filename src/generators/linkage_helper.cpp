// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file linkage_helper.cpp

#include "linkage_helper.h"

#include <stdexcept>

namespace data_generator::generator {

std::optional<std::string> parse_linkage_key(const Json& column) {
    if (!column.contains("data_linkage")) { return std::nullopt; }
    const auto& value = column.at("data_linkage");
    if (value.is_boolean()) { return value.get<bool>() ? std::optional<std::string>("default") : std::nullopt; }
    if (value.is_string()) {
        auto key = value.get<std::string>();
        if (key.empty()) { return std::nullopt; }
        return key;
    }
    throw std::invalid_argument("data_linkage must be a boolean or a string");
}

}  // namespace data_generator::generator
