// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file override_rules.cpp

#include "override_rules.h"

#include <random>
#include <stdexcept>

namespace data_generator::generator {

namespace {

void validate_percent(const int percent, const std::string& name) {
    if (percent < 0 || percent > 100) { throw std::invalid_argument(name + " percent must be in [0, 100]"); }
}

bool roll_remaining(int& remaining, const int rows_left) {
    if (remaining <= 0 || rows_left <= 0) { return false; }
    if (remaining >= rows_left) {
        --remaining;
        return true;
    }
    thread_local std::mt19937_64 rng{std::random_device{}()};
    std::uniform_int_distribution<int> dist(1, rows_left);
    if (dist(rng) <= remaining) {
        --remaining;
        return true;
    }
    return false;
}

bool roll_percent(const int percent) {
    if (percent <= 0) { return false; }
    if (percent >= 100) { return true; }
    thread_local std::mt19937_64 rng{std::random_device{}()};
    std::uniform_int_distribution<int> dist(1, 100);
    return dist(rng) <= percent;
}

}  // namespace

OverrideState parse_overrides(const Json& column) {
    OverrideState overrides;

    if (column.contains("default_value")) {
        const auto& cfg                 = column.at("default_value");
        overrides.default_rule.enabled = cfg.value("enabled", false);
        overrides.default_rule.percent = cfg.value("percent", 100);
        validate_percent(overrides.default_rule.percent, "default_value");
        if (cfg.contains("value")) { overrides.default_rule.value = cfg.at("value").get<std::string>(); }
    }

    if (column.contains("null_value")) {
        const auto& cfg              = column.at("null_value");
        overrides.null_rule.enabled = cfg.value("enabled", false);
        overrides.null_rule.percent = cfg.value("percent", 100);
        validate_percent(overrides.null_rule.percent, "null_value");
    }

    overrides.total_rows = column.value("rows", 0);
    if (overrides.total_rows < 0) { overrides.total_rows = 0; }
    if (column.contains("null_value_string")) {
        const auto& null_value = column.at("null_value_string");
        if (null_value.is_null()) {
            overrides.null_literal.clear();
        } else if (null_value.is_string()) {
            overrides.null_literal = null_value.get<std::string>();
        } else {
            throw std::invalid_argument("null_value_string must be null or a string");
        }
    }

    if (overrides.total_rows > 0) {
        if (overrides.default_rule.enabled && overrides.null_rule.enabled &&
            overrides.default_rule.percent + overrides.null_rule.percent > 100) {
            throw std::invalid_argument("default_value percent + null_value percent must be <= 100");
        }
        overrides.null_remaining =
            overrides.null_rule.enabled ? (overrides.total_rows * overrides.null_rule.percent) / 100 : 0;
        overrides.default_remaining =
            overrides.default_rule.enabled ? (overrides.total_rows * overrides.default_rule.percent) / 100 : 0;
    }

    return overrides;
}

std::optional<std::string> apply_override(OverrideState& state) {
    if (state.total_rows > 0) {
        const int rows_left = state.total_rows - state.row_index;
        if (state.null_rule.enabled && roll_remaining(state.null_remaining, rows_left)) {
            if (!state.null_literal.empty()) { return state.null_literal; }
            return std::string();
        }
        if (state.default_rule.enabled && roll_remaining(state.default_remaining, rows_left)) {
            return state.default_rule.value;
        }
        return std::nullopt;
    }

    if (state.null_rule.enabled && roll_percent(state.null_rule.percent)) {
        if (!state.null_literal.empty()) { return state.null_literal; }
        return std::string();
    }
    if (state.default_rule.enabled && roll_percent(state.default_rule.percent)) { return state.default_rule.value; }
    return std::nullopt;
}

void next_row(OverrideState& state) {
    if (state.total_rows > 0 && state.row_index < state.total_rows) { ++state.row_index; }
}

}  // namespace data_generator::generator
