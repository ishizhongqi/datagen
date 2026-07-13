// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file linkage_helper.cpp

#include "generators/core/linkage_helper.h"

#include <stdexcept>

namespace datagen::generator {

std::optional<std::string> parse_linkage_key(const Json& field) {
    if (!field.contains("group")) { return std::nullopt; }
    const auto& value = field.at("group");
    if (value.is_string()) {
        auto key = value.get<std::string>();
        if (key.empty()) { return std::nullopt; }
        return key;
    }
    throw std::invalid_argument("group must be a string");
}

}  // namespace datagen::generator
