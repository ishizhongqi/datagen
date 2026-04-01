// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file override_rules.cpp

#include "override_rules.h"

#include <random>
#include <stdexcept>

namespace data_generator::generator {

namespace {

void validate_percentage(const int percentage, const std::string& name) {
    if (percentage < 0 || percentage > 100) {
        throw std::invalid_argument(name + " percentage must be in [0, 100]");
    }
}

bool roll_remaining(int& remaining, const int rows_left) {
    if (remaining <= 0 || rows_left <= 0) { return false; }
    if (remaining >= rows_left) {
        --remaining;
        return true;
    }
    thread_local std::mt19937_64       rng{std::random_device{}()};
    std::uniform_int_distribution<int> dist(1, rows_left);
    if (dist(rng) <= remaining) {
        --remaining;
        return true;
    }
    return false;
}

bool roll_percentage(const int percentage) {
    if (percentage <= 0) { return false; }
    if (percentage >= 100) { return true; }
    thread_local std::mt19937_64       rng{std::random_device{}()};
    std::uniform_int_distribution<int> dist(1, 100);
    return dist(rng) <= percentage;
}

int read_percentage_value(const Json& config) {
    return config.value("percentage", 100);
}

}  // namespace

OverrideState parse_overrides(const Json& filed) {
    OverrideState overrides;

    if (filed.contains("default_value")) {
        const auto& cfg                = filed.at("default_value");
        overrides.default_rule.enabled = cfg.value("enabled", false);
        overrides.default_rule.percentage = read_percentage_value(cfg);
        validate_percentage(overrides.default_rule.percentage, "default_value");
        if (cfg.contains("value")) { overrides.default_rule.value = cfg.at("value").get<std::string>(); }
    }

    if (filed.contains("null_value")) {
        const auto& cfg             = filed.at("null_value");
        overrides.null_rule.enabled = cfg.value("enabled", false);
        overrides.null_rule.percentage = read_percentage_value(cfg);
        validate_percentage(overrides.null_rule.percentage, "null_value");
        if (overrides.null_rule.enabled) { overrides.null_literal = std::string(kNullSentinel); }
    }

    overrides.total_rows = filed.value("rows", 0);
    if (overrides.total_rows < 0) { overrides.total_rows = 0; }

    if (overrides.total_rows > 0) {
        if (overrides.default_rule.enabled &&
            overrides.null_rule.enabled &&
            overrides.default_rule.percentage +
            overrides.null_rule.percentage > 100) {
            throw std::invalid_argument("default_value percentage + null_value percentage must be <= 100");
        }
        overrides.null_remaining =
            overrides.null_rule.enabled ? (overrides.total_rows * overrides.null_rule.percentage) / 100 : 0;
        overrides.default_remaining =
            overrides.default_rule.enabled ? (overrides.total_rows * overrides.default_rule.percentage) / 100 : 0;
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

    if (state.null_rule.enabled && roll_percentage(state.null_rule.percentage)) {
        if (!state.null_literal.empty()) { return state.null_literal; }
        return std::string();
    }
    if (state.default_rule.enabled && roll_percentage(state.default_rule.percentage)) {
        return state.default_rule.value;
    }
    return std::nullopt;
}

void next_row(OverrideState& state) {
    if (state.total_rows > 0 && state.row_index < state.total_rows) { ++state.row_index; }
}

}  // namespace data_generator::generator
