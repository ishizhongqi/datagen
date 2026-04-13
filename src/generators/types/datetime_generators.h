// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file datetime_generators.h

#ifndef DATAGEN_DATETIME_GENERATORS_H
#define DATAGEN_DATETIME_GENERATORS_H

#include "generators/core/generator_registry.h"

namespace datagen::generator {

/// @brief Register datetime-related generators.
void register_datetime_generators(GeneratorRegistry& registry);

}  // namespace datagen::generator

#endif  // DATAGEN_DATETIME_GENERATORS_H
