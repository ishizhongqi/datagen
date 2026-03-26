// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file person_generators.cpp

#include "person_generators.h"

#include <faker/faker.h>

#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "generators/core/array_parser.h"
#include "generators/core/generator_base.h"
#include "generators/core/generator_registry.h"
#include "generators/core/linkage_helper.h"
#include "generators/core/override_rules.h"

namespace data_generator::generator {

namespace {

class PersonContext {
public:
    PersonContext(
        const faker::Genders                                    genders,
        const faker::Languages                                  languages,
        const faker::Regions                                    regions,
        const std::optional<std::span<const std::string_view>>& domains,
        const bool                                              unique
    ) :
        person_(genders, languages, regions, domains, unique) {}

    void next() {
        if (!next_called_) {
            person_.reroll();
            next_called_ = true;
        }
    }

    void reset() {
        next_called_ = false;
    }

    faker::person::Person& person() {
        return person_;
    }

private:
    faker::person::Person person_;
    bool                  next_called_ = false;
};

class SharedPersonContext {
public:
    void merge_config(const Json& config, const Json& filed, const std::string& generator_name) {
        if (config.contains("genders")) {
            const auto parsed = parse_genders(config);
            if (genders_.has_value() && genders_.value() != parsed) {
                throw std::runtime_error(
                    "Conflicting genders in linked person generators (found in " + generator_name + ")"
                );
            }
            genders_ = parsed;
        }
        if (config.contains("languages")) {
            const auto parsed = parse_languages(config);
            if (languages_.has_value() && languages_.value() != parsed) {
                throw std::runtime_error(
                    "Conflicting languages in linked person generators (found in " + generator_name + ")"
                );
            }
            languages_ = parsed;
        }
        if (config.contains("regions")) {
            const auto parsed = parse_regions(config);
            if (regions_.has_value() && regions_.value() != parsed) {
                throw std::runtime_error(
                    "Conflicting regions in linked person generators (found in " + generator_name + ")"
                );
            }
            regions_ = parsed;
        }
        if (config.contains("domains")) {
            const auto parsed = parse_string_array("domains", config);
            if (domains_.has_value() && domains_.value() != parsed) {
                throw std::runtime_error(
                    "Conflicting domains in linked person generators (found in " + generator_name + ")"
                );
            }
            domains_ = parsed;
        }
        if (filed.contains("unique")) {
            const auto parsed = filed.at("unique").get<bool>();
            unique_           = unique_.has_value() ? (unique_.value() || parsed) : parsed;
        }
    }

    PersonContext& ensure_context([[maybe_unused]] const std::string& generator_name) {
        if (!context_) {
            const auto genders   = genders_.value_or(faker::Genders::M | faker::Genders::F);
            const auto languages = languages_.value_or(faker::Languages::English);
            const auto regions   = regions_.value_or(faker::Regions::UnitedStates);
            const auto unique    = unique_.value_or(false);

            std::optional<std::span<const std::string_view>> domains = std::nullopt;
            if (domains_.has_value()) {
                domain_views_.clear();
                domain_views_.reserve(domains_->size());
                for (const auto& domain : domains_.value()) { domain_views_.emplace_back(domain); }
                domains = std::span<const std::string_view>(domain_views_);
            }

            context_ = std::make_shared<PersonContext>(genders, languages, regions, domains, unique);
        }
        return *context_;
    }

    void reset() const {
        if (context_) { context_->reset(); }
    }

private:
    std::optional<faker::Genders>           genders_;
    std::optional<faker::Languages>         languages_;
    std::optional<faker::Regions>           regions_;
    std::optional<std::vector<std::string>> domains_;
    std::optional<bool>                     unique_;
    std::vector<std::string_view>           domain_views_;
    std::shared_ptr<PersonContext>          context_;
};

class FirstNameGenerator : public IGenerator {
public:
    FirstNameGenerator(
        std::shared_ptr<SharedPersonContext> context,
        faker::Languages                     languages,
        faker::Genders                       genders,
        bool                                 linkage,
        bool                                 use_translation,
        OverrideState                        overrides
    ) :
        context_(std::move(context)),
        languages_(languages),
        genders_(genders),
        linkage_(linkage),
        use_translation_(use_translation),
        overrides_(std::move(overrides)) {}

    std::string generate() override {
        if (auto overridden = apply_override(overrides_)) { return *overridden; }
        if (linkage_) {
            auto& context = context_->ensure_context("first_name");
            context.next();
            return use_translation_ ? context.person().first_name().translation()
                                    : context.person().first_name().original();
        }
        auto bilingual = faker::person::first_name(languages_, genders_);
        return use_translation_ ? bilingual.translation() : bilingual.original();
    }

    void next() override {
        if (linkage_ && context_) { context_->reset(); }
        next_row(overrides_);
    }

private:
    std::shared_ptr<SharedPersonContext> context_;
    faker::Languages                     languages_;
    faker::Genders                       genders_;
    bool                                 linkage_;
    bool                                 use_translation_;
    OverrideState                        overrides_;
};

class LastNameGenerator : public IGenerator {
public:
    LastNameGenerator(
        std::shared_ptr<SharedPersonContext> context,
        faker::Languages                     languages,
        bool                                 linkage,
        bool                                 use_translation,
        OverrideState                        overrides
    ) :
        context_(std::move(context)),
        languages_(languages),
        linkage_(linkage),
        use_translation_(use_translation),
        overrides_(std::move(overrides)) {}

    std::string generate() override {
        if (auto overridden = apply_override(overrides_)) { return *overridden; }
        if (linkage_) {
            auto& context = context_->ensure_context("last_name");
            context.next();
            return use_translation_ ? context.person().last_name().translation()
                                    : context.person().last_name().original();
        }
        auto bilingual = faker::person::last_name(languages_);
        return use_translation_ ? bilingual.translation() : bilingual.original();
    }

    void next() override {
        if (linkage_ && context_) { context_->reset(); }
        next_row(overrides_);
    }

private:
    std::shared_ptr<SharedPersonContext> context_;
    faker::Languages                     languages_;
    bool                                 linkage_;
    bool                                 use_translation_;
    OverrideState                        overrides_;
};

class FullNameGenerator : public IGenerator {
public:
    FullNameGenerator(
        std::shared_ptr<SharedPersonContext> context,
        faker::Languages                     languages,
        faker::Genders                       genders,
        bool                                 linkage,
        bool                                 use_translation,
        OverrideState                        overrides
    ) :
        context_(std::move(context)),
        languages_(languages),
        genders_(genders),
        linkage_(linkage),
        use_translation_(use_translation),
        overrides_(std::move(overrides)) {}

    std::string generate() override {
        if (auto overridden = apply_override(overrides_)) { return *overridden; }
        if (linkage_) {
            auto& context = context_->ensure_context("full_name");
            context.next();
            return use_translation_ ? context.person().full_name().translation()
                                    : context.person().full_name().original();
        }
        auto bilingual = faker::person::full_name(languages_, genders_);
        return use_translation_ ? bilingual.translation() : bilingual.original();
    }

    void next() override {
        if (linkage_ && context_) { context_->reset(); }
        next_row(overrides_);
    }

private:
    std::shared_ptr<SharedPersonContext> context_;
    faker::Languages                     languages_;
    faker::Genders                       genders_;
    bool                                 linkage_;
    bool                                 use_translation_;
    OverrideState                        overrides_;
};

class GenderGenerator : public IGenerator {
public:
    GenderGenerator(
        std::shared_ptr<SharedPersonContext> context,
        faker::Languages                     languages,
        bool                                 linkage,
        OverrideState                        overrides
    ) :
        context_(std::move(context)), languages_(languages), linkage_(linkage), overrides_(std::move(overrides)) {}

    std::string generate() override {
        if (auto overridden = apply_override(overrides_)) { return *overridden; }
        if (linkage_) {
            auto& context = context_->ensure_context("gender");
            context.next();
            return context.person().gender();
        }
        return faker::person::gender(languages_);
    }

    void next() override {
        if (linkage_ && context_) { context_->reset(); }
        next_row(overrides_);
    }

private:
    std::shared_ptr<SharedPersonContext> context_;
    faker::Languages                     languages_;
    bool                                 linkage_;
    OverrideState                        overrides_;
};

class TitleGenerator : public IGenerator {
public:
    TitleGenerator(
        std::shared_ptr<SharedPersonContext> context,
        faker::Languages                     languages,
        faker::Genders                       genders,
        bool                                 linkage,
        OverrideState                        overrides
    ) :
        context_(std::move(context)),
        languages_(languages),
        genders_(genders),
        linkage_(linkage),
        overrides_(std::move(overrides)) {}

    std::string generate() override {
        if (auto overridden = apply_override(overrides_)) { return *overridden; }
        if (linkage_) {
            auto& context = context_->ensure_context("title");
            context.next();
            return context.person().title();
        }
        return faker::person::title(languages_, genders_);
    }

    void next() override {
        if (linkage_ && context_) { context_->reset(); }
        next_row(overrides_);
    }

private:
    std::shared_ptr<SharedPersonContext> context_;
    faker::Languages                     languages_;
    faker::Genders                       genders_;
    bool                                 linkage_;
    OverrideState                        overrides_;
};

class MaritalStatusGenerator : public IGenerator {
public:
    MaritalStatusGenerator(
        std::shared_ptr<SharedPersonContext> context,
        faker::Languages                     languages,
        bool                                 linkage,
        OverrideState                        overrides
    ) :
        context_(std::move(context)), languages_(languages), linkage_(linkage), overrides_(std::move(overrides)) {}

    std::string generate() override {
        if (auto overridden = apply_override(overrides_)) { return *overridden; }
        if (linkage_) {
            auto& context = context_->ensure_context("marital_status");
            context.next();
            return context.person().marital_status();
        }
        return faker::person::marital_status(languages_);
    }

    void next() override {
        if (linkage_ && context_) { context_->reset(); }
        next_row(overrides_);
    }

private:
    std::shared_ptr<SharedPersonContext> context_;
    faker::Languages                     languages_;
    bool                                 linkage_;
    OverrideState                        overrides_;
};

class PhoneNumberGenerator : public IGenerator {
public:
    PhoneNumberGenerator(
        std::shared_ptr<SharedPersonContext> context,
        faker::Regions                       regions,
        bool                                 is_international,
        bool                                 include_delimiters,
        bool                                 unique,
        bool                                 linkage,
        OverrideState                        overrides
    ) :
        context_(std::move(context)),
        regions_(regions),
        is_international_(is_international),
        include_delimiters_(include_delimiters),
        unique_(unique),
        linkage_(linkage),
        overrides_(std::move(overrides)) {}

    std::string generate() override {
        if (auto overridden = apply_override(overrides_)) { return *overridden; }
        if (linkage_) {
            auto& context = context_->ensure_context("phone_number");
            context.next();
            return context.person().phone_number(is_international_, include_delimiters_);
        }
        return faker::person::phone_number(is_international_, include_delimiters_, regions_, unique_);
    }

    void next() override {
        if (linkage_ && context_) { context_->reset(); }
        next_row(overrides_);
    }

private:
    std::shared_ptr<SharedPersonContext> context_;
    faker::Regions                       regions_;
    bool                                 is_international_;
    bool                                 include_delimiters_;
    bool                                 unique_;
    bool                                 linkage_;
    OverrideState                        overrides_;
};

class EmailGenerator : public IGenerator {
public:
    EmailGenerator(
        std::shared_ptr<SharedPersonContext> context,
        faker::Languages                     languages,
        std::vector<std::string>             domains,
        bool                                 unique,
        bool                                 linkage,
        OverrideState                        overrides
    ) :
        context_(std::move(context)),
        languages_(languages),
        domains_(std::move(domains)),
        unique_(unique),
        linkage_(linkage),
        overrides_(std::move(overrides)) {}

    void init_views() {
        if (!domain_views_.empty()) { return; }
        domain_views_.reserve(domains_.size());
        for (const auto& domain : domains_) { domain_views_.emplace_back(domain); }
    }

    std::string generate() override {
        if (auto overridden = apply_override(overrides_)) { return *overridden; }
        if (linkage_) {
            auto& context = context_->ensure_context("email");
            context.next();
            return context.person().email();
        }
        if (domains_.empty()) { return faker::person::email(languages_, std::nullopt, unique_); }
        init_views();
        return faker::person::email(languages_, std::span<const std::string_view>(domain_views_), unique_);
    }

    void next() override {
        if (linkage_ && context_) { context_->reset(); }
        next_row(overrides_);
    }

private:
    std::shared_ptr<SharedPersonContext> context_;
    faker::Languages                     languages_;
    std::vector<std::string>             domains_;
    std::vector<std::string_view>        domain_views_;
    bool                                 unique_;
    bool                                 linkage_;
    OverrideState                        overrides_;
};

class JobTitleGenerator : public IGenerator {
public:
    JobTitleGenerator(
        std::shared_ptr<SharedPersonContext> context,
        faker::Languages                     languages,
        bool                                 linkage,
        OverrideState                        overrides
    ) :
        context_(std::move(context)), languages_(languages), linkage_(linkage), overrides_(std::move(overrides)) {}

    std::string generate() override {
        if (auto overridden = apply_override(overrides_)) { return *overridden; }
        if (linkage_) {
            auto& context = context_->ensure_context("job_title");
            context.next();
            return context.person().job_title();
        }
        return faker::person::job_title(languages_);
    }

    void next() override {
        if (linkage_ && context_) { context_->reset(); }
        next_row(overrides_);
    }

private:
    std::shared_ptr<SharedPersonContext> context_;
    faker::Languages                     languages_;
    bool                                 linkage_;
    OverrideState                        overrides_;
};

class SocialNetworkIdGenerator : public IGenerator {
public:
    SocialNetworkIdGenerator(
        std::shared_ptr<SharedPersonContext> context,
        faker::Languages                     languages,
        bool                                 unique,
        bool                                 linkage,
        bool                                 use_translation,
        OverrideState                        overrides
    ) :
        context_(std::move(context)),
        languages_(languages),
        unique_(unique),
        linkage_(linkage),
        use_translation_(use_translation),
        overrides_(std::move(overrides)) {}

    std::string generate() override {
        if (auto overridden = apply_override(overrides_)) { return *overridden; }
        if (linkage_) {
            auto& context = context_->ensure_context("social_network_id");
            context.next();
            return use_translation_ ? context.person().social_network_id().translation()
                                    : context.person().social_network_id().original();
        }
        auto bilingual = faker::person::social_network_id(languages_, unique_);
        return use_translation_ ? bilingual.translation() : bilingual.original();
    }

    void next() override {
        if (linkage_ && context_) { context_->reset(); }
        next_row(overrides_);
    }

private:
    std::shared_ptr<SharedPersonContext> context_;
    faker::Languages                     languages_;
    bool                                 unique_;
    bool                                 linkage_;
    bool                                 use_translation_;
    OverrideState                        overrides_;
};

}  // namespace

void register_person_generators(GeneratorRegistry& registry) {
    auto shared_person_contexts =
        std::make_shared<std::unordered_map<std::string, std::shared_ptr<SharedPersonContext>>>();

    registry.register_generator("first_name", [shared_person_contexts](const Json& filed) {
        const Json& config          = filed.at("config");
        const auto  overrides       = parse_overrides(filed);
        const auto  languages       = parse_languages(config);
        const auto  genders         = parse_genders(config);
        const bool  use_translation = parse_use_translation(config);

        std::shared_ptr<SharedPersonContext> context;
        const auto                           linkage_key = parse_linkage_key(filed);
        if (linkage_key.has_value()) {
            auto& entry = (*shared_person_contexts)[*linkage_key];
            if (!entry) { entry = std::make_shared<SharedPersonContext>(); }
            entry->merge_config(config, filed, "first_name");
            context = entry;
        }
        const bool linkage = static_cast<bool>(context);
        return std::make_unique<FirstNameGenerator>(context, languages, genders, linkage, use_translation, overrides);
    });

    registry.register_generator("last_name", [shared_person_contexts](const Json& filed) {
        const Json& config          = filed.at("config");
        const auto  overrides       = parse_overrides(filed);
        const auto  languages       = parse_languages(config);
        const bool  use_translation = parse_use_translation(config);

        std::shared_ptr<SharedPersonContext> context;
        const auto                           linkage_key = parse_linkage_key(filed);
        if (linkage_key.has_value()) {
            auto& entry = (*shared_person_contexts)[*linkage_key];
            if (!entry) { entry = std::make_shared<SharedPersonContext>(); }
            entry->merge_config(config, filed, "last_name");
            context = entry;
        }
        const bool linkage = static_cast<bool>(context);
        return std::make_unique<LastNameGenerator>(context, languages, linkage, use_translation, overrides);
    });

    registry.register_generator("full_name", [shared_person_contexts](const Json& filed) {
        const Json& config          = filed.at("config");
        const auto  overrides       = parse_overrides(filed);
        const auto  languages       = parse_languages(config);
        const auto  genders         = parse_genders(config);
        const bool  use_translation = parse_use_translation(config);

        std::shared_ptr<SharedPersonContext> context;
        const auto                           linkage_key = parse_linkage_key(filed);
        if (linkage_key.has_value()) {
            auto& entry = (*shared_person_contexts)[*linkage_key];
            if (!entry) { entry = std::make_shared<SharedPersonContext>(); }
            entry->merge_config(config, filed, "full_name");
            context = entry;
        }
        const bool linkage = static_cast<bool>(context);
        return std::make_unique<FullNameGenerator>(context, languages, genders, linkage, use_translation, overrides);
    });

    registry.register_generator("gender", [shared_person_contexts](const Json& filed) {
        const Json& config    = filed.at("config");
        const auto  overrides = parse_overrides(filed);
        const auto  languages = parse_languages(config);

        std::shared_ptr<SharedPersonContext> context;
        const auto                           linkage_key = parse_linkage_key(filed);
        if (linkage_key.has_value()) {
            auto& entry = (*shared_person_contexts)[*linkage_key];
            if (!entry) { entry = std::make_shared<SharedPersonContext>(); }
            entry->merge_config(config, filed, "gender");
            context = entry;
        }
        const bool linkage = static_cast<bool>(context);
        return std::make_unique<GenderGenerator>(context, languages, linkage, overrides);
    });

    registry.register_generator("title", [shared_person_contexts](const Json& filed) {
        const Json& config    = filed.at("config");
        const auto  overrides = parse_overrides(filed);
        const auto  languages = parse_languages(config);
        const auto  genders   = parse_genders(config);

        std::shared_ptr<SharedPersonContext> context;
        const auto                           linkage_key = parse_linkage_key(filed);
        if (linkage_key.has_value()) {
            auto& entry = (*shared_person_contexts)[*linkage_key];
            if (!entry) { entry = std::make_shared<SharedPersonContext>(); }
            entry->merge_config(config, filed, "title");
            context = entry;
        }
        const bool linkage = static_cast<bool>(context);
        return std::make_unique<TitleGenerator>(context, languages, genders, linkage, overrides);
    });

    registry.register_generator("marital_status", [shared_person_contexts](const Json& filed) {
        const Json& config    = filed.at("config");
        const auto  overrides = parse_overrides(filed);
        const auto  languages = parse_languages(config);

        std::shared_ptr<SharedPersonContext> context;
        const auto                           linkage_key = parse_linkage_key(filed);
        if (linkage_key.has_value()) {
            auto& entry = (*shared_person_contexts)[*linkage_key];
            if (!entry) { entry = std::make_shared<SharedPersonContext>(); }
            entry->merge_config(config, filed, "marital_status");
            context = entry;
        }
        const bool linkage = static_cast<bool>(context);
        return std::make_unique<MaritalStatusGenerator>(context, languages, linkage, overrides);
    });

    registry.register_generator("phone_number", [shared_person_contexts](const Json& filed) {
        const bool  unique             = filed.value("unique", false);
        const Json& config             = filed.at("config");
        const auto  overrides          = parse_overrides(filed);
        const auto  regions            = parse_regions(config);
        const bool  is_international   = config.value("is_international", false);
        const bool  include_delimiters = config.value("include_delimiters", true);

        std::shared_ptr<SharedPersonContext> context;
        const auto                           linkage_key = parse_linkage_key(filed);
        if (linkage_key.has_value()) {
            auto& entry = (*shared_person_contexts)[*linkage_key];
            if (!entry) { entry = std::make_shared<SharedPersonContext>(); }
            entry->merge_config(config, filed, "phone_number");
            context = entry;
        }
        const bool linkage = static_cast<bool>(context);
        return std::make_unique<PhoneNumberGenerator>(
            context,
            regions,
            is_international,
            include_delimiters,
            unique,
            linkage,
            overrides
        );
    });

    registry.register_generator("email", [shared_person_contexts](const Json& filed) {
        const bool  unique    = filed.value("unique", false);
        const Json& config    = filed.at("config");
        const auto  overrides = parse_overrides(filed);
        const auto  languages = parse_languages(config);
        const auto  domains   = parse_string_array("domains", config);

        std::shared_ptr<SharedPersonContext> context;
        const auto                           linkage_key = parse_linkage_key(filed);
        if (linkage_key.has_value()) {
            auto& entry = (*shared_person_contexts)[*linkage_key];
            if (!entry) { entry = std::make_shared<SharedPersonContext>(); }
            entry->merge_config(config, filed, "email");
            context = entry;
        }
        const bool linkage = static_cast<bool>(context);
        return std::make_unique<EmailGenerator>(context, languages, domains, unique, linkage, overrides);
    });

    registry.register_generator("job_title", [shared_person_contexts](const Json& filed) {
        const Json& config    = filed.at("config");
        const auto  overrides = parse_overrides(filed);
        const auto  languages = parse_languages(config);

        std::shared_ptr<SharedPersonContext> context;
        const auto                           linkage_key = parse_linkage_key(filed);
        if (linkage_key.has_value()) {
            auto& entry = (*shared_person_contexts)[*linkage_key];
            if (!entry) { entry = std::make_shared<SharedPersonContext>(); }
            entry->merge_config(config, filed, "job_title");
            context = entry;
        }
        const bool linkage = static_cast<bool>(context);
        return std::make_unique<JobTitleGenerator>(context, languages, linkage, overrides);
    });

    registry.register_generator("social_network_id", [shared_person_contexts](const Json& filed) {
        const bool  unique          = filed.value("unique", false);
        const Json& config          = filed.at("config");
        const auto  overrides       = parse_overrides(filed);
        const auto  languages       = parse_languages(config);
        const bool  use_translation = parse_use_translation(config);

        std::shared_ptr<SharedPersonContext> context;
        const auto                           linkage_key = parse_linkage_key(filed);
        if (linkage_key.has_value()) {
            auto& entry = (*shared_person_contexts)[*linkage_key];
            if (!entry) { entry = std::make_shared<SharedPersonContext>(); }
            entry->merge_config(config, filed, "social_network_id");
            context = entry;
        }
        const bool linkage = static_cast<bool>(context);
        return std::make_unique<SocialNetworkIdGenerator>(
            context,
            languages,
            unique,
            linkage,
            use_translation,
            overrides
        );
    });
}

}  // namespace data_generator::generator
