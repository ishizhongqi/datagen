// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file computer_generators.cpp

#include "computer_generators.h"

#include <faker/faker.h>

#include <memory>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "generators/core/array_parser.h"
#include "generators/core/generator_base.h"
#include "generators/core/generator_registry.h"
#include "generators/core/linkage_helper.h"
#include "generators/core/override_rules.h"

namespace datagen::generator {

namespace {

/// ===============================
/// ip_address generator
/// ===============================
class IpAddressGenerator : public IGenerator {
public:
    explicit IpAddressGenerator(const Json& config, const bool unique, OverrideState overrides) :
        overrides_(std::move(overrides)) {
        ip_type_ = config.value("ip_address_type", "IPv4");
        unique_  = unique;
    }

    std::string generate() override {
        if (auto overridden = apply_override(overrides_)) { return *overridden; }
        if (ip_type_ == "IPv6") { return faker::computer::ip_address(faker::IpAddressType::IPv6, unique_); }
        return faker::computer::ip_address(faker::IpAddressType::IPv4, unique_);
    }

    void next() override {
        next_row(overrides_);
    }

private:
    std::string   ip_type_;
    bool          unique_;
    OverrideState overrides_;
};

/// ===============================
/// mac_address generator
/// ===============================
class MacAddressGenerator : public IGenerator {
public:
    explicit MacAddressGenerator(const bool unique, OverrideState overrides) :
        unique_(unique), overrides_(std::move(overrides)) {}

    std::string generate() override {
        if (auto overridden = apply_override(overrides_)) { return *overridden; }
        return faker::computer::mac_address(unique_);
    }

    void next() override {
        next_row(overrides_);
    }

private:
    bool          unique_;
    OverrideState overrides_;
};

/// ===============================
/// File context
/// ===============================
class FileContext {
public:
    FileContext(const faker::OperatingSystems operating_systems, const std::vector<std::string>& extensions) {
        std::vector<std::string_view> extension_views;
        extension_views.reserve(extensions.size());
        for (const auto& extension : extensions) { extension_views.emplace_back(extension); }
        file_        = faker::computer::File(operating_systems, extension_views);
        next_called_ = false;
    }

    void next() {
        if (!next_called_) {
            file_.reroll();
            next_called_ = true;
        }
    }

    void reset() {
        next_called_ = false;
    }

    faker::computer::File& file() {
        return file_;
    }

private:
    faker::computer::File file_;
    bool                  next_called_;
};

class SharedFileContext {
public:
    void merge_config(const Json& config, const std::string& generator_name) {
        if (config.contains("operating_systems")) {
            const auto parsed = parse_operating_systems(config);
            if (operating_systems_.has_value() && operating_systems_.value() != parsed) {
                throw std::runtime_error(
                    "Conflicting operating_systems in linked file generators (found in " + generator_name + ")"
                );
            }
            operating_systems_ = parsed;
        }

        if (config.contains("extensions")) {
            const auto parsed = parse_string_array("extensions", config);
            if (extensions_.has_value() && extensions_.value() != parsed) {
                throw std::runtime_error(
                    "Conflicting extensions in linked file generators (found in " + generator_name + ")"
                );
            }
            extensions_ = parsed;
        }
    }

    FileContext& ensure_context(const std::string& generator_name) {
        if (!context_) {
            if (!operating_systems_.has_value() || !extensions_.has_value()) {
                throw std::runtime_error(
                    "Linked file generators require both operating_systems and extensions before use (missing in " +
                    generator_name +
                    ")"
                );
            }
            context_ = std::make_shared<FileContext>(operating_systems_.value(), extensions_.value());
        }
        return *context_;
    }

    void reset() const {
        if (context_) { context_->reset(); }
    }

private:
    std::optional<faker::OperatingSystems>  operating_systems_;
    std::optional<std::vector<std::string>> extensions_;
    std::shared_ptr<FileContext>            context_;
};

/// ===============================
/// file_path generator
/// ===============================
class FilePathGenerator : public IGenerator {
public:
    FilePathGenerator(
        std::shared_ptr<SharedFileContext> context,
        const faker::OperatingSystems      operating_systems,
        std::vector<std::string>           extensions,
        const bool                         linkage,
        OverrideState                      overrides
    ) :
        context_(std::move(context)),
        operating_systems_(operating_systems),
        extensions_(std::move(extensions)),
        linkage_(linkage),
        overrides_(std::move(overrides)) {}

    void init_views() {
        if (!extension_views_.empty()) { return; }
        extension_views_.reserve(extensions_.size());
        for (const auto& ext : extensions_) { extension_views_.emplace_back(ext); }
    }

    std::string generate() override {
        if (auto overridden = apply_override(overrides_)) { return *overridden; }
        if (linkage_) {
            auto& context = context_->ensure_context("file_path");
            context.next();
            return context.file().path();
        }

        init_views();

        return faker::computer::file_path(operating_systems_, std::span<const std::string_view>(extension_views_));
    }

    void next() override {
        if (linkage_ && context_) { context_->reset(); }
        next_row(overrides_);
    }

private:
    std::shared_ptr<SharedFileContext> context_;
    faker::OperatingSystems            operating_systems_;
    std::vector<std::string>           extensions_;
    std::vector<std::string_view>      extension_views_;
    bool                               linkage_;
    OverrideState                      overrides_;
};

/// ===============================
/// file_directory generator
/// ===============================
class FileDirectoryGenerator : public IGenerator {
public:
    FileDirectoryGenerator(
        std::shared_ptr<SharedFileContext> context,
        const faker::OperatingSystems      operating_systems,
        const bool                         linkage,
        OverrideState                      overrides
    ) :
        context_(std::move(context)),
        operating_systems_(operating_systems),
        linkage_(linkage),
        overrides_(std::move(overrides)) {}

    std::string generate() override {
        if (auto overridden = apply_override(overrides_)) { return *overridden; }
        if (linkage_) {
            auto& context = context_->ensure_context("file_directory");
            context.next();
            return context.file().directory();
        }
        return faker::computer::file_directory(operating_systems_);
    }

    void next() override {
        if (linkage_ && context_) { context_->reset(); }
        next_row(overrides_);
    }

private:
    std::shared_ptr<SharedFileContext> context_;
    faker::OperatingSystems            operating_systems_;
    bool                               linkage_;
    OverrideState                      overrides_;
};

/// ===============================
/// file_name generator
/// ===============================
class FileNameGenerator : public IGenerator {
public:
    FileNameGenerator(
        std::shared_ptr<SharedFileContext> context,
        std::vector<std::string>           extensions,
        const bool                         linkage,
        OverrideState                      overrides
    ) :
        context_(std::move(context)),
        extensions_(std::move(extensions)),
        linkage_(linkage),
        overrides_(std::move(overrides)) {}

    void init_views() {
        if (!extension_views_.empty()) { return; }
        extension_views_.reserve(extensions_.size());
        for (const auto& ext : extensions_) { extension_views_.emplace_back(ext); }
    }

    std::string generate() override {
        if (auto overridden = apply_override(overrides_)) { return *overridden; }
        if (linkage_) {
            auto& context = context_->ensure_context("file_name");
            context.next();
            return context.file().name();
        }
        init_views();
        return faker::computer::file_name(std::span<const std::string_view>(extension_views_));
    }

    void next() override {
        if (linkage_ && context_) { context_->reset(); }
        next_row(overrides_);
    }

private:
    std::shared_ptr<SharedFileContext> context_;
    std::vector<std::string>           extensions_;
    std::vector<std::string_view>      extension_views_;
    bool                               linkage_;
    OverrideState                      overrides_;
};

/// ===============================
/// file_extension generator
/// ===============================
class FileExtensionGenerator : public IGenerator {
public:
    FileExtensionGenerator(
        std::shared_ptr<SharedFileContext> context,
        std::vector<std::string>           extensions,
        const bool                         linkage,
        OverrideState                      overrides
    ) :
        context_(std::move(context)),
        extensions_(std::move(extensions)),
        linkage_(linkage),
        overrides_(std::move(overrides)) {}

    void init_views() {
        if (!extension_views_.empty()) { return; }
        extension_views_.reserve(extensions_.size());
        for (const auto& ext : extensions_) { extension_views_.emplace_back(ext); }
    }

    std::string generate() override {
        if (auto overridden = apply_override(overrides_)) { return *overridden; }
        if (linkage_) {
            auto& context = context_->ensure_context("file_extension");
            context.next();
            return context.file().extension();
        }
        init_views();
        return faker::computer::file_extension(std::span<const std::string_view>(extension_views_));
    }

    void next() override {
        if (linkage_ && context_) { context_->reset(); }
        next_row(overrides_);
    }

private:
    std::shared_ptr<SharedFileContext> context_;
    std::vector<std::string>           extensions_;
    std::vector<std::string_view>      extension_views_;
    bool                               linkage_;
    OverrideState                      overrides_;
};

/// ===============================
/// url generator
/// ===============================
class UrlGenerator : public IGenerator {
public:
    UrlGenerator(const Json& config, const bool unique, OverrideState overrides) :
        subdomains_(parse_string_array("subdomains", config)), unique_(unique), overrides_(std::move(overrides)) {
        if (!config.contains("tlds")) { throw std::invalid_argument("url generator requires tlds"); }
        tlds_ = parse_string_array("tlds", config);
    }

    void init_views() {
        if (subdomain_views_.empty()) {
            subdomain_views_.reserve(subdomains_.size());
            for (const auto& subdomain : subdomains_) { subdomain_views_.emplace_back(subdomain); }
        }
        if (tld_views_.empty()) {
            tld_views_.reserve(tlds_.size());
            for (const auto& tld : tlds_) { tld_views_.emplace_back(tld); }
        }
    }

    std::string generate() override {
        if (auto overridden = apply_override(overrides_)) { return *overridden; }
        init_views();
        return faker::computer::url(
            std::span<const std::string_view>(subdomain_views_),
            std::span<const std::string_view>(tld_views_),
            unique_
        );
    }

    void next() override {
        next_row(overrides_);
    }

private:
    std::vector<std::string>      subdomains_;
    std::vector<std::string>      tlds_;
    std::vector<std::string_view> subdomain_views_;
    std::vector<std::string_view> tld_views_;
    bool                          unique_;
    OverrideState                 overrides_;
};

/// ===============================
/// hostname generator
/// ===============================
class HostnameGenerator : public IGenerator {
public:
    HostnameGenerator(const Json& config, const bool unique, OverrideState overrides) :
        subdomains_(parse_string_array("subdomains", config)), unique_(unique), overrides_(std::move(overrides)) {
        if (!config.contains("tlds")) { throw std::invalid_argument("hostname generator requires tlds"); }
        tlds_ = parse_string_array("tlds", config);
    }

    void init_views() {
        if (subdomain_views_.empty()) {
            subdomain_views_.reserve(subdomains_.size());
            for (const auto& subdomain : subdomains_) { subdomain_views_.emplace_back(subdomain); }
        }
        if (tld_views_.empty()) {
            tld_views_.reserve(tlds_.size());
            for (const auto& tld : tlds_) { tld_views_.emplace_back(tld); }
        }
    }

    std::string generate() override {
        if (auto overridden = apply_override(overrides_)) { return *overridden; }
        init_views();
        return faker::computer::hostname(
            std::span<const std::string_view>(subdomain_views_),
            std::span<const std::string_view>(tld_views_),
            unique_
        );
    }

    void next() override {
        next_row(overrides_);
    }

private:
    std::vector<std::string>      subdomains_;
    std::vector<std::string>      tlds_;
    std::vector<std::string_view> subdomain_views_;
    std::vector<std::string_view> tld_views_;
    bool                          unique_;
    OverrideState                 overrides_;
};

}  // namespace

/// ===============================
/// register
/// ===============================
void register_computer_generators(GeneratorRegistry& registry) {
    registry.register_generator("ip_address", [](const Json& filed) {
        const bool  unique    = filed.value("unique", true);
        const Json& config    = filed.at("config");
        const auto  overrides = parse_overrides(filed);
        return std::make_unique<IpAddressGenerator>(config, unique, overrides);
    });

    registry.register_generator("mac_address", [](const Json& filed) {
        const bool unique    = filed.value("unique", true);
        const auto overrides = parse_overrides(filed);
        return std::make_unique<MacAddressGenerator>(unique, overrides);
    });

    auto shared_file_contexts = std::make_shared<std::unordered_map<std::string, std::shared_ptr<SharedFileContext>>>();

    registry.register_generator("file_path", [shared_file_contexts](const Json& filed) {
        const Json& config = filed.at("config");

        const auto operating_systems = parse_operating_systems(config);
        const auto extensions        = parse_string_array("extensions", config);
        const auto overrides         = parse_overrides(filed);

        std::shared_ptr<SharedFileContext> context;
        const auto                         linkage_key = parse_linkage_key(filed);
        if (linkage_key.has_value()) {
            auto& entry = (*shared_file_contexts)[*linkage_key];
            if (!entry) { entry = std::make_shared<SharedFileContext>(); }
            entry->merge_config(config, "file_path");
            context = entry;
        }

        const bool linkage = static_cast<bool>(context);
        return std::make_unique<FilePathGenerator>(context, operating_systems, extensions, linkage, overrides);
    });

    registry.register_generator("file_directory", [shared_file_contexts](const Json& filed) {
        const Json& config = filed.at("config");

        const auto operating_systems = parse_operating_systems(config);
        const auto overrides         = parse_overrides(filed);

        std::shared_ptr<SharedFileContext> context;
        const auto                         linkage_key = parse_linkage_key(filed);
        if (linkage_key.has_value()) {
            auto& entry = (*shared_file_contexts)[*linkage_key];
            if (!entry) { entry = std::make_shared<SharedFileContext>(); }
            entry->merge_config(config, "file_directory");
            context = entry;
        }

        const bool linkage = static_cast<bool>(context);
        return std::make_unique<FileDirectoryGenerator>(context, operating_systems, linkage, overrides);
    });

    registry.register_generator("file_name", [shared_file_contexts](const Json& filed) {
        const Json& config = filed.at("config");

        const auto extensions = parse_string_array("extensions", config);
        const auto overrides  = parse_overrides(filed);

        std::shared_ptr<SharedFileContext> context;
        const auto                         linkage_key = parse_linkage_key(filed);
        if (linkage_key.has_value()) {
            auto& entry = (*shared_file_contexts)[*linkage_key];
            if (!entry) { entry = std::make_shared<SharedFileContext>(); }
            entry->merge_config(config, "file_name");
            context = entry;
        }

        const bool linkage = static_cast<bool>(context);
        return std::make_unique<FileNameGenerator>(context, extensions, linkage, overrides);
    });

    registry.register_generator("file_extension", [shared_file_contexts](const Json& filed) {
        const Json& config = filed.at("config");

        const auto extensions = parse_string_array("extensions", config);
        const auto overrides  = parse_overrides(filed);

        std::shared_ptr<SharedFileContext> context;
        const auto                         linkage_key = parse_linkage_key(filed);
        if (linkage_key.has_value()) {
            auto& entry = (*shared_file_contexts)[*linkage_key];
            if (!entry) { entry = std::make_shared<SharedFileContext>(); }
            entry->merge_config(config, "file_extension");
            context = entry;
        }

        const bool linkage = static_cast<bool>(context);
        return std::make_unique<FileExtensionGenerator>(context, extensions, linkage, overrides);
    });

    registry.register_generator("url", [](const Json& filed) {
        const bool  unique    = filed.value("unique", false);
        const Json& config    = filed.at("config");
        const auto  overrides = parse_overrides(filed);
        return std::make_unique<UrlGenerator>(config, unique, overrides);
    });

    registry.register_generator("hostname", [](const Json& filed) {
        const bool  unique    = filed.value("unique", false);
        const Json& config    = filed.at("config");
        const auto  overrides = parse_overrides(filed);
        return std::make_unique<HostnameGenerator>(config, unique, overrides);
    });
}

}  // namespace datagen::generator
