// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file product_generators.cpp

#include "product_generators.h"

#include <faker/faker.h>

#include <string>
#include <vector>

#include "array_parser.h"
#include "generator_base.h"
#include "generator_registry.h"
#include "override_rules.h"

namespace data_generator::generator {

namespace {

class ProductNameGenerator : public IGenerator {
public:
    ProductNameGenerator(faker::Languages languages, std::vector<std::string> keywords, OverrideState overrides) :
        languages_(languages), keywords_(std::move(keywords)), overrides_(std::move(overrides)) {}

    void init_views() {
        if (!keyword_views_.empty()) { return; }
        keyword_views_.reserve(keywords_.size());
        for (const auto& keyword : keywords_) { keyword_views_.emplace_back(keyword); }
    }

    std::string generate() override {
        if (auto overridden = apply_override(overrides_)) { return *overridden; }
        if (keywords_.empty()) { return faker::product::product_name(languages_, std::nullopt); }
        init_views();
        return faker::product::product_name(languages_, std::span<const std::string_view>(keyword_views_));
    }

    void next() override {
        next_row(overrides_);
    }

private:
    faker::Languages              languages_;
    std::vector<std::string>      keywords_;
    std::vector<std::string_view> keyword_views_;
    OverrideState                 overrides_;
};

class ProductCategoryGenerator : public IGenerator {
public:
    ProductCategoryGenerator(faker::Languages languages, OverrideState overrides) :
        languages_(languages), overrides_(std::move(overrides)) {}

    std::string generate() override {
        if (auto overridden = apply_override(overrides_)) { return *overridden; }
        return faker::product::product_category(languages_);
    }

    void next() override {
        next_row(overrides_);
    }

private:
    faker::Languages languages_;
    OverrideState    overrides_;
};

class ColorGenerator : public IGenerator {
public:
    ColorGenerator(faker::Languages languages, OverrideState overrides) :
        languages_(languages), overrides_(std::move(overrides)) {}

    std::string generate() override {
        if (auto overridden = apply_override(overrides_)) { return *overridden; }
        return faker::product::color(languages_);
    }

    void next() override {
        next_row(overrides_);
    }

private:
    faker::Languages languages_;
    OverrideState    overrides_;
};

class SizeGenerator : public IGenerator {
public:
    explicit SizeGenerator(OverrideState overrides) : overrides_(std::move(overrides)) {}

    std::string generate() override {
        if (auto overridden = apply_override(overrides_)) { return *overridden; }
        return faker::product::size();
    }

    void next() override {
        next_row(overrides_);
    }

private:
    OverrideState overrides_;
};

class BarcodeGenerator : public IGenerator {
public:
    BarcodeGenerator(faker::BarcodeTypes barcode_types, bool unique, OverrideState overrides) :
        barcode_types_(barcode_types), unique_(unique), overrides_(std::move(overrides)) {}

    std::string generate() override {
        if (auto overridden = apply_override(overrides_)) { return *overridden; }
        return faker::product::barcode(barcode_types_, unique_);
    }

    void next() override {
        next_row(overrides_);
    }

private:
    faker::BarcodeTypes barcode_types_;
    bool                unique_;
    OverrideState       overrides_;
};

}  // namespace

void register_product_generators(GeneratorRegistry& registry) {
    registry.register_generator("product_name", [](const Json& filed) {
        const Json& config    = filed.at("config");
        const auto  overrides = parse_overrides(filed);
        const auto  languages = parse_languages(config);
        const auto  keywords  = parse_string_array("keywords", config);
        return std::make_unique<ProductNameGenerator>(languages, keywords, overrides);
    });

    registry.register_generator("product_category", [](const Json& filed) {
        const Json& config    = filed.at("config");
        const auto  overrides = parse_overrides(filed);
        const auto  languages = parse_languages(config);
        return std::make_unique<ProductCategoryGenerator>(languages, overrides);
    });

    registry.register_generator("color", [](const Json& filed) {
        const Json& config    = filed.at("config");
        const auto  overrides = parse_overrides(filed);
        const auto  languages = parse_languages(config);
        return std::make_unique<ColorGenerator>(languages, overrides);
    });

    registry.register_generator("size", [](const Json& filed) {
        const auto overrides = parse_overrides(filed);
        return std::make_unique<SizeGenerator>(overrides);
    });

    registry.register_generator("barcode", [](const Json& filed) {
        const bool  unique        = filed.value("unique", false);
        const Json& config        = filed.at("config");
        const auto  overrides     = parse_overrides(filed);
        const auto  barcode_types = parse_barcode_type(config);
        return std::make_unique<BarcodeGenerator>(barcode_types, unique, overrides);
    });
}

}  // namespace data_generator::generator
