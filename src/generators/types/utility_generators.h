// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file utility_generators.h

#ifndef DATA_GENERATOR_UTILITY_GENERATORS_H
#define DATA_GENERATOR_UTILITY_GENERATORS_H

#include "generators/core/generator_registry.h"

namespace data_generator::generator {

/// @brief Register utility generators (boolean, sequence, regular_expression).
void register_utility_generators(GeneratorRegistry& registry);

}  // namespace data_generator::generator

#endif  // DATA_GENERATOR_UTILITY_GENERATORS_H
