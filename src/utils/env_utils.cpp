// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file env_utils.cpp

#include "utils/env_utils.h"

#include <cstdlib>

namespace data_generator::utils {

std::optional<std::string> get_env_value(const char* key) {
    if (!key || *key == '\0') { return std::nullopt; }
#if defined(_WIN32)
    char*  value  = nullptr;
    size_t length = 0;
    if (_dupenv_s(&value, &length, key) != 0 || value == nullptr || length == 0) {
        if (value != nullptr) { std::free(value); }
        return std::nullopt;
    }
    std::string text(value);
    std::free(value);
    return text;
#else
    const char* value = std::getenv(key);
    if (!value || *value == '\0') { return std::nullopt; }
    return std::string(value);
#endif
}

std::string get_env_or_default(const char* key, const char* default_value) {
    if (const auto value = get_env_value(key); value.has_value()) { return *value; }
    return default_value ? default_value : "";
}

}  // namespace data_generator::utils
