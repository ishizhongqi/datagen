// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file number_generators.cpp

#include "number_generators.h"

#include <faker/faker.h>

#include <iomanip>
#include <sstream>
#include <string>

#include "generator_base.h"
#include "generator_registry.h"
#include "override_rules.h"

namespace data_generator::generator {

namespace {

class IntegerGenerator : public IGenerator {
public:
    IntegerGenerator(int64_t start, int64_t end, OverrideState overrides) :
        start_(start), end_(end), overrides_(std::move(overrides)) {}

    std::string generate() override {
        if (auto overridden = apply_override(overrides_)) { return *overridden; }
        return std::to_string(faker::number::integer<int64_t>(start_, end_));
    }

    void next() override { next_row(overrides_); }

private:
    int64_t      start_;
    int64_t      end_;
    OverrideState overrides_;
};

class UnsignedIntegerGenerator : public IGenerator {
public:
    UnsignedIntegerGenerator(uint64_t start, uint64_t end, OverrideState overrides) :
        start_(start), end_(end), overrides_(std::move(overrides)) {}

    std::string generate() override {
        if (auto overridden = apply_override(overrides_)) { return *overridden; }
        return std::to_string(faker::number::unsigned_integer<uint64_t>(start_, end_));
    }

    void next() override { next_row(overrides_); }

private:
    uint64_t     start_;
    uint64_t     end_;
    OverrideState overrides_;
};

class DecimalGenerator : public IGenerator {
public:
    DecimalGenerator(double start, double end, int decimal_places, OverrideState overrides) :
        start_(start), end_(end), decimal_places_(decimal_places), overrides_(std::move(overrides)) {}

    std::string generate() override {
        if (auto overridden = apply_override(overrides_)) { return *overridden; }
        const double value = faker::number::decimal<double>(start_, end_, decimal_places_);
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(decimal_places_) << value;
        return oss.str();
    }

    void next() override { next_row(overrides_); }

private:
    double       start_;
    double       end_;
    int          decimal_places_;
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

    void next() override { next_row(overrides_); }

private:
    double       start_;
    double       end_;
    int          decimal_places_;
    OverrideState overrides_;
};

}  // namespace

void register_number_generators(GeneratorRegistry& registry) {
    registry.register_generator("integer", [](const Json& column) {
        const Json& config = column.at("config");
        const auto overrides = parse_overrides(column);
        const int64_t start = config.value("start", 0);
        const int64_t end   = config.value("end", 100);
        return std::make_unique<IntegerGenerator>(start, end, overrides);
    });

    registry.register_generator("unsigned_integer", [](const Json& column) {
        const Json& config = column.at("config");
        const auto overrides = parse_overrides(column);
        const uint64_t start = config.value("start", 0);
        const uint64_t end   = config.value("end", 100);
        return std::make_unique<UnsignedIntegerGenerator>(start, end, overrides);
    });

    registry.register_generator("decimal", [](const Json& column) {
        const Json& config = column.at("config");
        const auto overrides = parse_overrides(column);
        const double start = config.value("start", 0.0);
        const double end   = config.value("end", 100.0);
        const int decimal_places = config.value("decimal_places", 2);
        return std::make_unique<DecimalGenerator>(start, end, decimal_places, overrides);
    });

    registry.register_generator("decimal_string", [](const Json& column) {
        const Json& config = column.at("config");
        const auto overrides = parse_overrides(column);
        const double start = config.value("start", 0.0);
        const double end   = config.value("end", 100.0);
        const int decimal_places = config.value("decimal_places", 2);
        return std::make_unique<DecimalStringGenerator>(start, end, decimal_places, overrides);
    });
}

}  // namespace data_generator::generator
