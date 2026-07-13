// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file utility_generators.h

#ifndef DATAGEN_UTILITY_GENERATORS_H
#define DATAGEN_UTILITY_GENERATORS_H

#include "generators/core/generator_registry.h"

namespace datagen::generator {

/// @brief Register utility generators (boolean, sequence, regular_expression).
void register_utility_generators(GeneratorRegistry& registry);

}  // namespace datagen::generator

#endif  // DATAGEN_UTILITY_GENERATORS_H
