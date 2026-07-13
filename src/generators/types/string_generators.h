// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file string_generators.h

#ifndef DATAGEN_STRING_GENERATORS_H
#define DATAGEN_STRING_GENERATORS_H

#include "generators/core/generator_registry.h"

namespace datagen::generator {

/// @brief Register string-related generators.
void register_string_generators(GeneratorRegistry& registry);

}  // namespace datagen::generator

#endif  // DATAGEN_STRING_GENERATORS_H
