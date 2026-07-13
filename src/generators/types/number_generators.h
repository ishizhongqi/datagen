// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file number_generators.h

#ifndef DATAGEN_NUMBER_GENERATORS_H
#define DATAGEN_NUMBER_GENERATORS_H

#include "generators/core/generator_registry.h"

namespace datagen::generator {

/// @brief Register number-related generators.
void register_number_generators(GeneratorRegistry& registry);

}  // namespace datagen::generator

#endif  // DATAGEN_NUMBER_GENERATORS_H
