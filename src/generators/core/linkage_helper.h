// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file linkage_helper.h

#ifndef DATAGEN_LINKAGE_HELPER_H
#define DATAGEN_LINKAGE_HELPER_H

#include <nlohmann/json.hpp>
#include <optional>
#include <string>

namespace datagen::generator {

using Json = nlohmann::json;

std::optional<std::string> parse_linkage_key(const Json& field);

}  // namespace datagen::generator

#endif  // DATAGEN_LINKAGE_HELPER_H
