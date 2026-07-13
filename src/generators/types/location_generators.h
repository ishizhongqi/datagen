// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file location_generators.h

#ifndef DATAGEN_LOCATION_GENERATORS_H
#define DATAGEN_LOCATION_GENERATORS_H

#include "generators/core/generator_registry.h"

namespace datagen::generator {

/// @brief Register location-related generators.
void register_location_generators(GeneratorRegistry& registry);

}  // namespace datagen::generator

#endif  // DATAGEN_LOCATION_GENERATORS_H
