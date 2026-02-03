// Copyright (c) 2026 Shizhongqi
// Licensed under the Apache License 2.0.
// See the LICENSE file in the project root for more information.

/// @file computer_generators.cpp

#include "computer_generators.h"

#include <faker/faker.h>

#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <vector>

#include "array_parser.h"
#include "generator_base.h"
#include "generator_registry.h"

namespace data_generator::generator {

namespace {

/// ===============================
/// ip_address generator
/// ===============================
class IpAddressGenerator : public IGenerator {
public:
    explicit IpAddressGenerator(const Json& config, const bool unique) {
        ip_type_ = config.value("ip_address_type", "IPv4");
        unique_  = unique;
    }

    std::string generate() override {
        if (ip_type_ == "IPv6") { return faker::computer::ip_address(faker::IpAddressType::IPv6, unique_); }
        return faker::computer::ip_address(faker::IpAddressType::IPv4, unique_);
    }

    void next() override {}

private:
    std::string ip_type_;
    bool        unique_;
};

/// ===============================
/// mac_address generator
/// ===============================
class MacAddressGenerator : public IGenerator {
public:
    explicit MacAddressGenerator(const bool unique) {
        unique_ = unique;
    }

    std::string generate() override {
        return faker::computer::mac_address(unique_);
    }

    void next() override {}

private:
    bool unique_;
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
        next_called_                                    = false;
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
                    generator_name + ")"
                );
            }
            context_ = std::make_shared<FileContext>(operating_systems_.value(), extensions_.value());
        }
        return *context_;
    }

    void reset() {
        if (context_) { context_->reset(); }
    }

private:
    std::optional<faker::OperatingSystems> operating_systems_;
    std::optional<std::vector<std::string>> extensions_;
    std::shared_ptr<FileContext> context_;
};

/// ===============================
/// file_path generator
/// ===============================
class FilePathGenerator : public IGenerator {
public:
    FilePathGenerator(
        std::shared_ptr<SharedFileContext> context,
        const faker::OperatingSystems operating_systems,
        std::vector<std::string>      extensions,
        const bool                    linkage
    ) :
        context_(std::move(context)),
        operating_systems_(operating_systems),
        extensions_(std::move(extensions)),
        linkage_(linkage) {}

    std::string generate() override {
        if (linkage_) {
            auto& context = context_->ensure_context("file_path");
            context.next();
            return context.file().path();
        }

        std::vector<std::string_view> extension_views;
        extension_views.reserve(extensions_.size());
        for (const auto& ext : extensions_) { extension_views.emplace_back(ext); }

        return faker::computer::file_path(operating_systems_, std::span<const std::string_view>(extension_views));
    }

    void next() override {
        if (linkage_ && context_) { context_->reset(); }
    }

private:
    std::shared_ptr<SharedFileContext> context_;
    faker::OperatingSystems      operating_systems_;
    std::vector<std::string>     extensions_;
    bool                         linkage_;
};

/// ===============================
/// file_directory generator
/// ===============================
class FileDirectoryGenerator : public IGenerator {
public:
    FileDirectoryGenerator(
        std::shared_ptr<SharedFileContext> context,
        const faker::OperatingSystems operating_systems,
        const bool                    linkage
    ) :
        context_(std::move(context)), operating_systems_(operating_systems), linkage_(linkage) {}

    std::string generate() override {
        if (linkage_) {
            auto& context = context_->ensure_context("file_directory");
            context.next();
            return context.file().directory();
        }
        return faker::computer::file_directory(operating_systems_);
    }

    void next() override {
        if (linkage_ && context_) { context_->reset(); }
    }

private:
    std::shared_ptr<SharedFileContext> context_;
    faker::OperatingSystems      operating_systems_;
    bool                         linkage_;
};

/// ===============================
/// file_name generator
/// ===============================
class FileNameGenerator : public IGenerator {
public:
    FileNameGenerator(
        std::shared_ptr<SharedFileContext> context,
        std::vector<std::string>           extensions,
        const bool                         linkage
    ) :
        context_(std::move(context)), extensions_(std::move(extensions)), linkage_(linkage) {}

    std::string generate() override {
        if (linkage_) {
            auto& context = context_->ensure_context("file_name");
            context.next();
            return context.file().name();
        }
        std::vector<std::string_view> extension_views;
        extension_views.reserve(extensions_.size());
        for (const auto& ext : extensions_) { extension_views.emplace_back(ext); }
        return faker::computer::file_name(std::span<const std::string_view>(extension_views));
    }

    void next() override {
        if (linkage_ && context_) { context_->reset(); }
    }

private:
    std::shared_ptr<SharedFileContext> context_;
    std::vector<std::string>     extensions_;
    bool                         linkage_;
};

/// ===============================
/// file_extension generator
/// ===============================
class FileExtensionGenerator : public IGenerator {
public:
    FileExtensionGenerator(
        std::shared_ptr<SharedFileContext> context,
        std::vector<std::string>     extensions,
        const bool                   linkage
    ) :
        context_(std::move(context)), extensions_(std::move(extensions)), linkage_(linkage) {}

    std::string generate() override {
        if (linkage_) {
            auto& context = context_->ensure_context("file_extension");
            context.next();
            return context.file().extension();
        }
        std::vector<std::string_view> extension_views;
        extension_views.reserve(extensions_.size());
        for (const auto& ext : extensions_) { extension_views.emplace_back(ext); }
        return faker::computer::file_extension(std::span<const std::string_view>(extension_views));
    }

    void next() override {
        if (linkage_ && context_) { context_->reset(); }
    }

private:
    std::shared_ptr<SharedFileContext> context_;
    std::vector<std::string>     extensions_;
    bool                         linkage_;
};

}  // namespace

/// ===============================
/// register
/// ===============================
void register_computer_generators(GeneratorRegistry& registry) {
    registry.register_generator("ip_address", [](const Json& column) {
        const bool  unique = column.value("unique", true);
        const Json& config = column.at("config");
        return std::make_unique<IpAddressGenerator>(config, unique);
    });

    registry.register_generator("mac_address", [](const Json& column) {
        const bool unique = column.value("unique", true);
        return std::make_unique<MacAddressGenerator>(unique);
    });

    auto shared_file_context = std::make_shared<SharedFileContext>();

    registry.register_generator("file_path", [shared_file_context](const Json& column) {
        const bool  linkage = column.value("data_linkage", true);
        const Json& config  = column.at("config");

        const auto operating_systems = parse_operating_systems(config);
        const auto extensions        = parse_string_array("extensions", config);

        if (linkage) {
            shared_file_context->merge_config(config, "file_path");
        }

        return std::make_unique<FilePathGenerator>(shared_file_context, operating_systems, extensions, linkage);
    });

    registry.register_generator("file_directory", [shared_file_context](const Json& column) {
        const bool  linkage = column.value("data_linkage", true);
        const Json& config  = column.at("config");

        const auto operating_systems = parse_operating_systems(config);

        if (linkage) {
            shared_file_context->merge_config(config, "file_directory");
        }
        return std::make_unique<FileDirectoryGenerator>(shared_file_context, operating_systems, linkage);
    });

    registry.register_generator("file_name", [shared_file_context](const Json& column) {
        const bool  linkage = column.value("data_linkage", true);
        const Json& config  = column.at("config");

        const auto extensions = parse_string_array("extensions", config);

        if (linkage) {
            shared_file_context->merge_config(config, "file_name");
        }
        return std::make_unique<FileNameGenerator>(shared_file_context, extensions, linkage);
    });

    registry.register_generator("file_extension", [shared_file_context](const Json& column) {
        const bool  linkage = column.value("data_linkage", true);
        const Json& config  = column.at("config");

        const auto extensions = parse_string_array("extensions", config);

        if (linkage) {
            shared_file_context->merge_config(config, "file_extension");
        }
        return std::make_unique<FileExtensionGenerator>(shared_file_context, extensions, linkage);
    });
}

}  // namespace data_generator::generator
