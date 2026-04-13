// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file computer_generators.h

#ifndef DATAGEN_COMPUTER_GENERATORS_H
#define DATAGEN_COMPUTER_GENERATORS_H

#include "generators/core/generator_registry.h"

namespace datagen::generator {

/// @brief Register computer-related generators.
void register_computer_generators(GeneratorRegistry& registry);

}  // namespace datagen::generator

#endif  // DATAGEN_COMPUTER_GENERATORS_H
