// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file string_generators.cpp

#include "string_generators.h"

#include <faker/faker.h>

#include <string>
#include <vector>

#include "generators/core/array_parser.h"
#include "generators/core/generator_base.h"
#include "generators/core/generator_registry.h"
#include "generators/core/override_rules.h"

namespace data_generator::generator {

namespace {

class EnumItemGenerator : public IGenerator {
public:
    EnumItemGenerator(std::vector<std::string> enums, OverrideState overrides) :
        enums_(std::move(enums)), overrides_(std::move(overrides)) {}

    void init_views() {
        if (!enum_views_.empty()) { return; }
        enum_views_.reserve(enums_.size());
        for (const auto& value : enums_) { enum_views_.emplace_back(value); }
    }

    std::string generate() override {
        if (auto overridden = apply_override(overrides_)) { return *overridden; }
        init_views();
        return faker::string::enum_item(std::span<const std::string_view>(enum_views_));
    }

    void next() override {
        next_row(overrides_);
    }

private:
    std::vector<std::string>      enums_;
    std::vector<std::string_view> enum_views_;
    OverrideState                 overrides_;
};

class TextGenerator : public IGenerator {
public:
    TextGenerator(unsigned int start, unsigned int end, OverrideState overrides) :
        start_(start), end_(end), overrides_(std::move(overrides)) {}

    std::string generate() override {
        if (auto overridden = apply_override(overrides_)) { return *overridden; }
        return faker::string::text(start_, end_);
    }

    void next() override {
        next_row(overrides_);
    }

private:
    unsigned int  start_;
    unsigned int  end_;
    OverrideState overrides_;
};

class UuidGenerator : public IGenerator {
public:
    UuidGenerator(bool include_hyphens, OverrideState overrides) :
        include_hyphens_(include_hyphens), overrides_(std::move(overrides)) {}

    std::string generate() override {
        if (auto overridden = apply_override(overrides_)) { return *overridden; }
        return faker::string::uuid(include_hyphens_);
    }

    void next() override {
        next_row(overrides_);
    }

private:
    bool          include_hyphens_;
    OverrideState overrides_;
};

}  // namespace

void register_string_generators(GeneratorRegistry& registry) {
    registry.register_generator("enum_item", [](const Json& filed) {
        const Json& config    = filed.at("config");
        const auto  overrides = parse_overrides(filed);
        const auto  enums     = parse_string_array("enums", config);
        return std::make_unique<EnumItemGenerator>(enums, overrides);
    });

    registry.register_generator("text", [](const Json& filed) {
        const Json&        config    = filed.at("config");
        const auto         overrides = parse_overrides(filed);
        const unsigned int start     = config.value("number_of_chars_start", 100U);
        const unsigned int end       = config.value("number_of_chars_end", 10000U);
        return std::make_unique<TextGenerator>(start, end, overrides);
    });

    registry.register_generator("uuid", [](const Json& filed) {
        const Json& config          = filed.at("config");
        const auto  overrides       = parse_overrides(filed);
        const bool  include_hyphens = config.value("include_hyphens", true);
        return std::make_unique<UuidGenerator>(include_hyphens, overrides);
    });
}

}  // namespace data_generator::generator
