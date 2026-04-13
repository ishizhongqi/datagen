// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file number_generators.cpp

#include "number_generators.h"

#include <faker/faker.h>

#include <string>

#include "generators/core/generator_base.h"
#include "generators/core/generator_registry.h"
#include "generators/core/override_rules.h"

namespace datagen::generator {

namespace {

class IntegerGenerator : public IGenerator {
public:
    IntegerGenerator(int64_t start, int64_t end, OverrideState overrides) :
        start_(start), end_(end), overrides_(std::move(overrides)) {}

    std::string generate() override {
        if (auto overridden = apply_override(overrides_)) { return *overridden; }
        return std::to_string(faker::number::integer<int64_t>(start_, end_));
    }

    void next() override {
        next_row(overrides_);
    }

private:
    int64_t       start_;
    int64_t       end_;
    OverrideState overrides_;
};

class DecimalStringGenerator : public IGenerator {
public:
    DecimalStringGenerator(double start, double end, int decimal_places, OverrideState overrides) :
        start_(start), end_(end), decimal_places_(decimal_places), overrides_(std::move(overrides)) {}

    std::string generate() override {
        if (auto overridden = apply_override(overrides_)) { return *overridden; }
        return faker::number::decimal_string<double>(start_, end_, decimal_places_);
    }

    void next() override {
        next_row(overrides_);
    }

private:
    double        start_;
    double        end_;
    int           decimal_places_;
    OverrideState overrides_;
};

}  // namespace

void register_number_generators(GeneratorRegistry& registry) {
    registry.register_generator("integer", [](const Json& filed) {
        const Json&   config    = filed.at("config");
        const auto    overrides = parse_overrides(filed);
        const int64_t start     = config.value("start", 0);
        const int64_t end       = config.value("end", 100);
        return std::make_unique<IntegerGenerator>(start, end, overrides);
    });

    registry.register_generator("decimal", [](const Json& filed) {
        const Json&  config         = filed.at("config");
        const auto   overrides      = parse_overrides(filed);
        const double start          = config.value("start", 0.0);
        const double end            = config.value("end", 100.0);
        const int    decimal_places = config.value("decimal_places", 2);
        return std::make_unique<DecimalStringGenerator>(start, end, decimal_places, overrides);
    });
}

}  // namespace datagen::generator
