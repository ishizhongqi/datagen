// Copyright (c) 2026 Shizhongqi
// Licensed under the Apache License 2.0.
// See the LICENSE file in the project root for more information.

/// @file generator_registry.cpp

#include "generator_registry.h"

#include <nlohmann/json.hpp>
#include <stdexcept>

#include "generator_base.h"

namespace data_generator::generator {

void GeneratorRegistry::register_generator(const std::string& name, GeneratorCreator creator) {
    creators_[name] = std::move(creator);
}

std::unique_ptr<IGenerator> GeneratorRegistry::create(const std::string& name, const Json& column) const {
    const auto it = creators_.find(name);
    if (it == creators_.end()) { throw std::runtime_error("Unknown generator: " + name); }
    return it->second(column);
}

}  // namespace data_generator::generator
