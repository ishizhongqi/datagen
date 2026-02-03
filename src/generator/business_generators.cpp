// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file business_generators.cpp

#include "business_generators.h"

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

class CompanyContext {
public:
    explicit CompanyContext(const faker::Languages languages) : company_(languages) {}

    void next() {
        if (!next_called_) {
            company_.reroll();
            next_called_ = true;
        }
    }

    void reset() {
        next_called_ = false;
    }

    faker::business::Company& company() {
        return company_;
    }

private:
    faker::business::Company company_;
    bool                     next_called_ = false;
};

class SharedCompanyContext {
public:
    void merge_config(const Json& config, const std::string& generator_name) {
        if (config.contains("languages")) {
            const auto parsed = parse_languages(config);
            if (languages_.has_value() && languages_.value() != parsed) {
                throw std::runtime_error(
                    "Conflicting languages in linked business generators (found in " + generator_name + ")"
                );
            }
            languages_ = parsed;
        }
    }

    CompanyContext& ensure_context([[maybe_unused]] const std::string& generator_name) {
        if (!context_) {
            const auto languages = languages_.value_or(faker::Languages::English);
            context_             = std::make_shared<CompanyContext>(languages);
        }
        return *context_;
    }

    void reset() {
        if (context_) { context_->reset(); }
    }

private:
    std::optional<faker::Languages> languages_;
    std::shared_ptr<CompanyContext> context_;
};

class CompanyNameGenerator : public IGenerator {
public:
    CompanyNameGenerator(
        std::shared_ptr<SharedCompanyContext> context,
        faker::Languages languages,
        bool linkage,
        bool use_translation,
        OverrideState overrides
    ) :
        context_(std::move(context)),
        languages_(languages),
        linkage_(linkage),
        use_translation_(use_translation),
        overrides_(std::move(overrides)) {}

    std::string generate() override {
        if (auto overridden = apply_override(overrides_)) { return *overridden; }
        if (linkage_) {
            auto& context = context_->ensure_context("company_name");
            context.next();
            return use_translation_ ? context.company().name().translation() : context.company().name().original();
        }
        auto bilingual = faker::business::company_name(languages_);
        return use_translation_ ? bilingual.translation() : bilingual.original();
    }

    void next() override {
        if (linkage_ && context_) { context_->reset(); }
        next_row(overrides_);
    }

private:
    std::shared_ptr<SharedCompanyContext> context_;
    faker::Languages                      languages_;
    bool                                  linkage_;
    bool                                  use_translation_;
    OverrideState                         overrides_;
};

class DepartmentGenerator : public IGenerator {
public:
    DepartmentGenerator(faker::Languages languages, OverrideState overrides) :
        languages_(languages), overrides_(std::move(overrides)) {}

    std::string generate() override {
        if (auto overridden = apply_override(overrides_)) { return *overridden; }
        return faker::business::department(languages_);
    }

    void next() override {
        next_row(overrides_);
    }

private:
    faker::Languages languages_;
    OverrideState    overrides_;
};

class IndustryGenerator : public IGenerator {
public:
    IndustryGenerator(
        std::shared_ptr<SharedCompanyContext> context,
        faker::Languages languages,
        bool linkage,
        OverrideState overrides
    ) :
        context_(std::move(context)),
        languages_(languages),
        linkage_(linkage),
        overrides_(std::move(overrides)) {}

    std::string generate() override {
        if (auto overridden = apply_override(overrides_)) { return *overridden; }
        if (linkage_) {
            auto& context = context_->ensure_context("industry");
            context.next();
            return context.company().industry();
        }
        return faker::business::industry(languages_);
    }

    void next() override {
        if (linkage_ && context_) { context_->reset(); }
        next_row(overrides_);
    }

private:
    std::shared_ptr<SharedCompanyContext> context_;
    faker::Languages                      languages_;
    bool                                  linkage_;
    OverrideState                         overrides_;
};

}  // namespace

void register_business_generators(GeneratorRegistry& registry) {
    auto shared_company_context = std::make_shared<SharedCompanyContext>();

    registry.register_generator("company_name", [shared_company_context](const Json& column) {
        const bool  linkage = column.value("data_linkage", true);
        const Json& config  = column.at("config");
        const auto  overrides = parse_overrides(column);
        const auto  languages = parse_languages(config);
        const bool  use_translation = parse_use_translation(config);

        if (linkage) { shared_company_context->merge_config(config, "company_name"); }
        return std::make_unique<CompanyNameGenerator>(
            shared_company_context,
            languages,
            linkage,
            use_translation,
            overrides
        );
    });

    registry.register_generator("department", [](const Json& column) {
        const Json& config  = column.at("config");
        const auto  overrides = parse_overrides(column);
        const auto  languages = parse_languages(config);
        return std::make_unique<DepartmentGenerator>(languages, overrides);
    });

    registry.register_generator("industry", [shared_company_context](const Json& column) {
        const bool  linkage = column.value("data_linkage", true);
        const Json& config  = column.at("config");
        const auto  overrides = parse_overrides(column);
        const auto  languages = parse_languages(config);
        if (linkage) { shared_company_context->merge_config(config, "industry"); }
        return std::make_unique<IndustryGenerator>(shared_company_context, languages, linkage, overrides);
    });
}

}  // namespace data_generator::generator
