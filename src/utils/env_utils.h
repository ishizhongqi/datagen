// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file env_utils.h

#ifndef DATA_GENERATOR_ENV_UTILS_H
#define DATA_GENERATOR_ENV_UTILS_H

#include <optional>
#include <string>

namespace data_generator::utils {

std::optional<std::string> get_env_value(const char* key);
std::string get_env_or_default(const char* key, const char* default_value);

}  // namespace data_generator::utils

#endif
