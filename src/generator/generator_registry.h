// Copyright (c) 2026 Shizhongqi
// Licensed under the Apache License 2.0.
// See the LICENSE file in the project root for more information.

/// @file generator_registry.h

#ifndef DATA_GENERATOR_GENERATOR_REGISTRY_H
#define DATA_GENERATOR_GENERATOR_REGISTRY_H

#include <functional>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>

#include "generator_base.h"

namespace data_generator::generator {

using Json = nlohmann::json;

/// @brief Factory function type for generators.
using GeneratorCreator = std::function<std::unique_ptr<IGenerator>(const Json& config)>;

/// @brief Registry for all generators.
class GeneratorRegistry {
public:
    /// @brief Register a generator creator.
    void register_generator(const std::string& name, GeneratorCreator creator);

    /// @brief Create generator by name.
    std::unique_ptr<IGenerator> create(const std::string& name, const Json& column) const;

private:
    std::unordered_map<std::string, GeneratorCreator> creators_;
};

}  // namespace data_generator::generator

#endif  // DATA_GENERATOR_GENERATOR_REGISTRY_H
