// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file override_rules.h

#ifndef DATA_GENERATOR_OVERRIDE_RULES_H
#define DATA_GENERATOR_OVERRIDE_RULES_H

#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <string_view>

namespace data_generator::generator {

using Json = nlohmann::json;

struct OverrideRule {
    bool        enabled = false;
    int         percent = 0;
    std::string value;
};

struct OverrideState {
    OverrideRule null_rule;
    OverrideRule default_rule;
    int          total_rows        = 0;
    int          row_index         = 0;
    int          null_remaining    = 0;
    int          default_remaining = 0;
    std::string  null_literal;
};

constexpr std::string_view kNullSentinel = "\x1e__DG_NULL__\x1e";

OverrideState parse_overrides(const Json& filed);

std::optional<std::string> apply_override(OverrideState& state);

void next_row(OverrideState& state);

}  // namespace data_generator::generator

#endif  // DATA_GENERATOR_OVERRIDE_RULES_H
