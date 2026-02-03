// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file location_generators.cpp

#include "location_generators.h"

#include <faker/faker.h>

#include <memory>
#include <optional>
#include <stdexcept>
#include <string>

#include "array_parser.h"
#include "generator_base.h"
#include "generator_registry.h"
#include "override_rules.h"

namespace data_generator::generator {

namespace {

class LocationContext {
public:
    explicit LocationContext(const faker::Regions regions) : location_(regions) {}

    void next() {
        if (!next_called_) {
            location_.reroll();
            next_called_ = true;
        }
    }

    void reset() {
        next_called_ = false;
    }

    faker::location::Location& location() {
        return location_;
    }

private:
    faker::location::Location location_;
    bool                       next_called_ = false;
};

class SharedLocationContext {
public:
    void merge_config(const Json& config, const std::string& generator_name) {
        if (config.contains("regions")) {
            const auto parsed = parse_regions(config);
            if (regions_.has_value() && regions_.value() != parsed) {
                throw std::runtime_error(
                    "Conflicting regions in linked location generators (found in " + generator_name + ")"
                );
            }
            regions_ = parsed;
        }
    }

    LocationContext& ensure_context([[maybe_unused]] const std::string& generator_name) {
        if (!context_) {
            const auto regions = regions_.value_or(faker::Regions::UnitedStates);
            context_           = std::make_shared<LocationContext>(regions);
        }
        return *context_;
    }

    void reset() {
        if (context_) { context_->reset(); }
    }

private:
    std::optional<faker::Regions>  regions_;
    std::shared_ptr<LocationContext> context_;
};

class AddressLine1Generator : public IGenerator {
public:
    AddressLine1Generator(
        std::shared_ptr<SharedLocationContext> context,
        faker::Regions regions,
        bool linkage,
        bool use_translation,
        OverrideState overrides
    ) :
        context_(std::move(context)),
        regions_(regions),
        linkage_(linkage),
        use_translation_(use_translation),
        overrides_(std::move(overrides)) {}

    std::string generate() override {
        if (auto overridden = apply_override(overrides_)) { return *overridden; }
        if (linkage_) {
            auto& context = context_->ensure_context("address_line1");
            context.next();
            return use_translation_ ? context.location().address_line1().translation()
                                    : context.location().address_line1().original();
        }
        auto bilingual = faker::location::address_line1(regions_);
        return use_translation_ ? bilingual.translation() : bilingual.original();
    }

    void next() override {
        if (linkage_ && context_) { context_->reset(); }
        next_row(overrides_);
    }

private:
    std::shared_ptr<SharedLocationContext> context_;
    faker::Regions                         regions_;
    bool                                   linkage_;
    bool                                   use_translation_;
    OverrideState                          overrides_;
};

class AddressLine2Generator : public IGenerator {
public:
    AddressLine2Generator(
        std::shared_ptr<SharedLocationContext> context,
        faker::Regions regions,
        bool linkage,
        bool use_translation,
        OverrideState overrides
    ) :
        context_(std::move(context)),
        regions_(regions),
        linkage_(linkage),
        use_translation_(use_translation),
        overrides_(std::move(overrides)) {}

    std::string generate() override {
        if (auto overridden = apply_override(overrides_)) { return *overridden; }
        if (linkage_) {
            auto& context = context_->ensure_context("address_line2");
            context.next();
            return use_translation_ ? context.location().address_line2().translation()
                                    : context.location().address_line2().original();
        }
        auto bilingual = faker::location::address_line2(regions_);
        return use_translation_ ? bilingual.translation() : bilingual.original();
    }

    void next() override {
        if (linkage_ && context_) { context_->reset(); }
        next_row(overrides_);
    }

private:
    std::shared_ptr<SharedLocationContext> context_;
    faker::Regions                         regions_;
    bool                                   linkage_;
    bool                                   use_translation_;
    OverrideState                          overrides_;
};

class PostcodeGenerator : public IGenerator {
public:
    PostcodeGenerator(std::shared_ptr<SharedLocationContext> context, faker::Regions regions, bool linkage, OverrideState overrides) :
        context_(std::move(context)), regions_(regions), linkage_(linkage), overrides_(std::move(overrides)) {}

    std::string generate() override {
        if (auto overridden = apply_override(overrides_)) { return *overridden; }
        if (linkage_) {
            auto& context = context_->ensure_context("postcode");
            context.next();
            return context.location().postcode();
        }
        return faker::location::postcode(regions_);
    }

    void next() override {
        if (linkage_ && context_) { context_->reset(); }
        next_row(overrides_);
    }

private:
    std::shared_ptr<SharedLocationContext> context_;
    faker::Regions                         regions_;
    bool                                   linkage_;
    OverrideState                          overrides_;
};

class FullAddressGenerator : public IGenerator {
public:
    FullAddressGenerator(
        std::shared_ptr<SharedLocationContext> context,
        faker::Regions regions,
        bool linkage,
        bool use_translation,
        OverrideState overrides
    ) :
        context_(std::move(context)),
        regions_(regions),
        linkage_(linkage),
        use_translation_(use_translation),
        overrides_(std::move(overrides)) {}

    std::string generate() override {
        if (auto overridden = apply_override(overrides_)) { return *overridden; }
        if (linkage_) {
            auto& context = context_->ensure_context("full_address");
            context.next();
            return use_translation_ ? context.location().full_address().translation()
                                    : context.location().full_address().original();
        }
        auto bilingual = faker::location::full_address(regions_);
        return use_translation_ ? bilingual.translation() : bilingual.original();
    }

    void next() override {
        if (linkage_ && context_) { context_->reset(); }
        next_row(overrides_);
    }

private:
    std::shared_ptr<SharedLocationContext> context_;
    faker::Regions                         regions_;
    bool                                   linkage_;
    bool                                   use_translation_;
    OverrideState                          overrides_;
};

class CityGenerator : public IGenerator {
public:
    CityGenerator(
        std::shared_ptr<SharedLocationContext> context,
        faker::Regions regions,
        bool linkage,
        bool use_translation,
        OverrideState overrides
    ) :
        context_(std::move(context)),
        regions_(regions),
        linkage_(linkage),
        use_translation_(use_translation),
        overrides_(std::move(overrides)) {}

    std::string generate() override {
        if (auto overridden = apply_override(overrides_)) { return *overridden; }
        if (linkage_) {
            auto& context = context_->ensure_context("city");
            context.next();
            return use_translation_ ? context.location().city().translation() : context.location().city().original();
        }
        auto bilingual = faker::location::city(regions_);
        return use_translation_ ? bilingual.translation() : bilingual.original();
    }

    void next() override {
        if (linkage_ && context_) { context_->reset(); }
        next_row(overrides_);
    }

private:
    std::shared_ptr<SharedLocationContext> context_;
    faker::Regions                         regions_;
    bool                                   linkage_;
    bool                                   use_translation_;
    OverrideState                          overrides_;
};

class RegionGenerator : public IGenerator {
public:
    RegionGenerator(faker::CountryCodesStandard standard, faker::Languages languages, OverrideState overrides) :
        standard_(standard), languages_(languages), overrides_(std::move(overrides)) {}

    std::string generate() override {
        if (auto overridden = apply_override(overrides_)) { return *overridden; }
        return faker::location::region(standard_, languages_);
    }

    void next() override { next_row(overrides_); }

private:
    faker::CountryCodesStandard standard_;
    faker::Languages            languages_;
    OverrideState               overrides_;
};

}  // namespace

void register_location_generators(GeneratorRegistry& registry) {
    auto shared_location_context = std::make_shared<SharedLocationContext>();

    registry.register_generator("address_line1", [shared_location_context](const Json& column) {
        const bool  linkage = column.value("data_linkage", true);
        const Json& config  = column.at("config");
        const auto  overrides = parse_overrides(column);
        const auto  regions = parse_regions(config);
        const bool  use_translation = parse_use_translation(config);

        if (linkage) { shared_location_context->merge_config(config, "address_line1"); }
        return std::make_unique<AddressLine1Generator>(
            shared_location_context,
            regions,
            linkage,
            use_translation,
            overrides
        );
    });

    registry.register_generator("address_line2", [shared_location_context](const Json& column) {
        const bool  linkage = column.value("data_linkage", true);
        const Json& config  = column.at("config");
        const auto  overrides = parse_overrides(column);
        const auto  regions = parse_regions(config);
        const bool  use_translation = parse_use_translation(config);

        if (linkage) { shared_location_context->merge_config(config, "address_line2"); }
        return std::make_unique<AddressLine2Generator>(
            shared_location_context,
            regions,
            linkage,
            use_translation,
            overrides
        );
    });

    registry.register_generator("postcode", [shared_location_context](const Json& column) {
        const bool  linkage = column.value("data_linkage", true);
        const Json& config  = column.at("config");
        const auto  overrides = parse_overrides(column);
        const auto  regions = parse_regions(config);

        if (linkage) { shared_location_context->merge_config(config, "postcode"); }
        return std::make_unique<PostcodeGenerator>(shared_location_context, regions, linkage, overrides);
    });

    registry.register_generator("full_address", [shared_location_context](const Json& column) {
        const bool  linkage = column.value("data_linkage", true);
        const Json& config  = column.at("config");
        const auto  overrides = parse_overrides(column);
        const auto  regions = parse_regions(config);
        const bool  use_translation = parse_use_translation(config);

        if (linkage) { shared_location_context->merge_config(config, "full_address"); }
        return std::make_unique<FullAddressGenerator>(
            shared_location_context,
            regions,
            linkage,
            use_translation,
            overrides
        );
    });

    registry.register_generator("city", [shared_location_context](const Json& column) {
        const bool  linkage = column.value("data_linkage", true);
        const Json& config  = column.at("config");
        const auto  overrides = parse_overrides(column);
        const auto  regions = parse_regions(config);
        const bool  use_translation = parse_use_translation(config);

        if (linkage) { shared_location_context->merge_config(config, "city"); }
        return std::make_unique<CityGenerator>(
            shared_location_context,
            regions,
            linkage,
            use_translation,
            overrides
        );
    });

    registry.register_generator("region", [](const Json& column) {
        const Json& config  = column.at("config");
        const auto  overrides = parse_overrides(column);
        const auto  standard = parse_country_codes_standard(config);
        const auto  languages = parse_languages(config);
        return std::make_unique<RegionGenerator>(standard, languages, overrides);
    });
}

}  // namespace data_generator::generator
