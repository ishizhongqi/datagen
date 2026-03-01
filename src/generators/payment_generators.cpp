// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file payment_generators.cpp

#include "payment_generators.h"

#include <faker/faker.h>

#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "array_parser.h"
#include "generator_base.h"
#include "generator_registry.h"
#include "linkage_helper.h"
#include "override_rules.h"

namespace data_generator::generator {

namespace {

class CardContext {
public:
    CardContext(
        const faker::Languages languages,
        const faker::CardTypes card_types,
        const std::string&     start_month,
        const std::string&     end_month,
        const bool             unique
    ) :
        card_(languages, card_types, start_month, end_month, unique) {}

    void next() {
        if (!next_called_) {
            card_.reroll();
            next_called_ = true;
        }
    }

    void reset() {
        next_called_ = false;
    }

    faker::payment::Card& card() {
        return card_;
    }

private:
    faker::payment::Card card_;
    bool                 next_called_ = false;
};

class SharedCardContext {
public:
    void merge_config(const Json& config, const Json& filed, const std::string& generator_name) {
        if (config.contains("languages")) {
            const auto parsed = parse_languages(config);
            if (languages_.has_value() && languages_.value() != parsed) {
                throw std::runtime_error(
                    "Conflicting languages in linked payment generators (found in " + generator_name + ")"
                );
            }
            languages_ = parsed;
        }
        if (config.contains("card_types")) {
            const auto parsed = parse_card_types(config);
            if (card_types_.has_value() && card_types_.value() != parsed) {
                throw std::runtime_error(
                    "Conflicting card_types in linked payment generators (found in " + generator_name + ")"
                );
            }
            card_types_ = parsed;
        }
        if (config.contains("start_month")) {
            const auto parsed = config.at("start_month").get<std::string>();
            if (start_month_.has_value() && start_month_.value() != parsed) {
                throw std::runtime_error(
                    "Conflicting start_month in linked payment generators (found in " + generator_name + ")"
                );
            }
            start_month_ = parsed;
        }
        if (config.contains("end_month")) {
            const auto parsed = config.at("end_month").get<std::string>();
            if (end_month_.has_value() && end_month_.value() != parsed) {
                throw std::runtime_error(
                    "Conflicting end_month in linked payment generators (found in " + generator_name + ")"
                );
            }
            end_month_ = parsed;
        }
        if (filed.contains("unique")) {
            const auto parsed = filed.at("unique").get<bool>();
            unique_ = unique_.has_value() ? (unique_.value() || parsed) : parsed;
        }
    }

    CardContext& ensure_context([[maybe_unused]] const std::string& generator_name) {
        if (!context_) {
            const auto languages  = languages_.value_or(faker::Languages::English);
            const auto card_types = card_types_.value_or(
                faker::CardTypes::AmericanExpress |
                faker::CardTypes::JCB |
                faker::CardTypes::MasterCard |
                faker::CardTypes::UnionPay |
                faker::CardTypes::Visa
            );
            const auto start_month = start_month_.value_or("01/00");
            const auto end_month   = end_month_.value_or("12/50");
            const auto unique      = unique_.value_or(false);
            context_ = std::make_shared<CardContext>(languages, card_types, start_month, end_month, unique);
        }
        return *context_;
    }

    void reset() const {
        if (context_) { context_->reset(); }
    }

private:
    std::optional<faker::Languages> languages_;
    std::optional<faker::CardTypes> card_types_;
    std::optional<std::string>      start_month_;
    std::optional<std::string>      end_month_;
    std::optional<bool>             unique_;
    std::shared_ptr<CardContext>    context_;
};

class PaymentMethodGenerator : public IGenerator {
public:
    PaymentMethodGenerator(
        std::shared_ptr<SharedCardContext> context,
        std::vector<std::string>           methods,
        bool                               linkage,
        OverrideState                      overrides
    ) :
        context_(std::move(context)),
        methods_(std::move(methods)),
        linkage_(linkage),
        overrides_(std::move(overrides)) {}

    void init_views() {
        if (!method_views_.empty()) { return; }
        method_views_.reserve(methods_.size());
        for (const auto& method : methods_) { method_views_.emplace_back(method); }
    }

    std::string generate() override {
        if (auto overridden = apply_override(overrides_)) { return *overridden; }
        if (linkage_) {
            auto& context = context_->ensure_context("payment_method");
            context.next();
            return context.card().payment_method();
        }
        if (methods_.empty()) { return faker::payment::payment_method(std::nullopt); }
        init_views();
        return faker::payment::payment_method(std::span<const std::string_view>(method_views_));
    }

    void next() override {
        if (linkage_ && context_) { context_->reset(); }
        next_row(overrides_);
    }

private:
    std::shared_ptr<SharedCardContext> context_;
    std::vector<std::string>           methods_;
    std::vector<std::string_view>      method_views_;
    bool                               linkage_;
    OverrideState                      overrides_;
};

class CardTypeGenerator : public IGenerator {
public:
    CardTypeGenerator(
        std::shared_ptr<SharedCardContext> context,
        faker::Languages                   languages,
        faker::CardTypes                   card_types,
        bool                               linkage,
        OverrideState                      overrides
    ) :
        context_(std::move(context)),
        languages_(languages),
        card_types_(card_types),
        linkage_(linkage),
        overrides_(std::move(overrides)) {}

    std::string generate() override {
        if (auto overridden = apply_override(overrides_)) { return *overridden; }
        if (linkage_) {
            auto& context = context_->ensure_context("card_type");
            context.next();
            return context.card().type();
        }
        return faker::payment::card_type(languages_, card_types_);
    }

    void next() override {
        if (linkage_ && context_) { context_->reset(); }
        next_row(overrides_);
    }

private:
    std::shared_ptr<SharedCardContext> context_;
    faker::Languages                   languages_;
    faker::CardTypes                   card_types_;
    bool                               linkage_;
    OverrideState                      overrides_;
};

class CardNumberGenerator : public IGenerator {
public:
    CardNumberGenerator(
        std::shared_ptr<SharedCardContext> context,
        faker::CardTypes                   card_types,
        bool                               unique,
        bool                               linkage,
        OverrideState                      overrides
    ) :
        context_(std::move(context)),
        card_types_(card_types),
        unique_(unique),
        linkage_(linkage),
        overrides_(std::move(overrides)) {}

    std::string generate() override {
        if (auto overridden = apply_override(overrides_)) { return *overridden; }
        if (linkage_) {
            auto& context = context_->ensure_context("card_number");
            context.next();
            return context.card().number();
        }
        return faker::payment::card_number(card_types_, unique_);
    }

    void next() override {
        if (linkage_ && context_) { context_->reset(); }
        next_row(overrides_);
    }

private:
    std::shared_ptr<SharedCardContext> context_;
    faker::CardTypes                   card_types_;
    bool                               unique_;
    bool                               linkage_;
    OverrideState                      overrides_;
};

class CardDateGenerator : public IGenerator {
public:
    CardDateGenerator(
        std::shared_ptr<SharedCardContext> context,
        std::string                        start_month,
        std::string                        end_month,
        bool                               linkage,
        OverrideState                      overrides
    ) :
        context_(std::move(context)),
        start_month_(std::move(start_month)),
        end_month_(std::move(end_month)),
        linkage_(linkage),
        overrides_(std::move(overrides)) {}

    std::string generate() override {
        if (auto overridden = apply_override(overrides_)) { return *overridden; }
        if (linkage_) {
            auto& context = context_->ensure_context("card_date");
            context.next();
            return context.card().date();
        }
        return faker::payment::card_date(start_month_, end_month_);
    }

    void next() override {
        if (linkage_ && context_) { context_->reset(); }
        next_row(overrides_);
    }

private:
    std::shared_ptr<SharedCardContext> context_;
    std::string                        start_month_;
    std::string                        end_month_;
    bool                               linkage_;
    OverrideState                      overrides_;
};

}  // namespace

void register_payment_generators(GeneratorRegistry& registry) {
    auto shared_card_contexts = std::make_shared<std::unordered_map<std::string, std::shared_ptr<SharedCardContext>>>();

    registry.register_generator("payment_method", [shared_card_contexts](const Json& filed) {
        const Json&                        config    = filed.at("config");
        const auto                         overrides = parse_overrides(filed);
        const auto                         methods   = parse_string_array("payment_methods", config);
        std::shared_ptr<SharedCardContext> context;
        const auto                         linkage_key = parse_linkage_key(filed);
        if (linkage_key.has_value()) {
            auto& entry = (*shared_card_contexts)[*linkage_key];
            if (!entry) { entry = std::make_shared<SharedCardContext>(); }
            entry->merge_config(config, filed, "payment_method");
            context = entry;
        }
        const bool linkage = static_cast<bool>(context);
        return std::make_unique<PaymentMethodGenerator>(context, methods, linkage, overrides);
    });

    registry.register_generator("card_type", [shared_card_contexts](const Json& filed) {
        const Json& config     = filed.at("config");
        const auto  overrides  = parse_overrides(filed);
        const auto  languages  = parse_languages(config);
        const auto  card_types = parse_card_types(config);

        std::shared_ptr<SharedCardContext> context;
        const auto                         linkage_key = parse_linkage_key(filed);
        if (linkage_key.has_value()) {
            auto& entry = (*shared_card_contexts)[*linkage_key];
            if (!entry) { entry = std::make_shared<SharedCardContext>(); }
            entry->merge_config(config, filed, "card_type");
            context = entry;
        }
        const bool linkage = static_cast<bool>(context);
        return std::make_unique<CardTypeGenerator>(context, languages, card_types, linkage, overrides);
    });

    registry.register_generator("card_number", [shared_card_contexts](const Json& filed) {
        const bool  unique     = filed.value("unique", false);
        const Json& config     = filed.at("config");
        const auto  overrides  = parse_overrides(filed);
        const auto  card_types = parse_card_types(config);

        std::shared_ptr<SharedCardContext> context;
        const auto                         linkage_key = parse_linkage_key(filed);
        if (linkage_key.has_value()) {
            auto& entry = (*shared_card_contexts)[*linkage_key];
            if (!entry) { entry = std::make_shared<SharedCardContext>(); }
            entry->merge_config(config, filed, "card_number");
            context = entry;
        }
        const bool linkage = static_cast<bool>(context);
        return std::make_unique<CardNumberGenerator>(context, card_types, unique, linkage, overrides);
    });

    registry.register_generator("card_date", [shared_card_contexts](const Json& filed) {
        const Json&       config      = filed.at("config");
        const auto        overrides   = parse_overrides(filed);
        const std::string start_month = config.value("start_month", "01/00");
        const std::string end_month   = config.value("end_month", "12/50");

        std::shared_ptr<SharedCardContext> context;
        const auto                         linkage_key = parse_linkage_key(filed);
        if (linkage_key.has_value()) {
            auto& entry = (*shared_card_contexts)[*linkage_key];
            if (!entry) { entry = std::make_shared<SharedCardContext>(); }
            entry->merge_config(config, filed, "card_date");
            context = entry;
        }
        const bool linkage = static_cast<bool>(context);
        return std::make_unique<CardDateGenerator>(context, start_month, end_month, linkage, overrides);
    });
}

}  // namespace data_generator::generator
