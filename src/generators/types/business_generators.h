// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file business_generators.h

#ifndef DATAGEN_BUSINESS_GENERATORS_H
#define DATAGEN_BUSINESS_GENERATORS_H

#include "generators/core/generator_registry.h"

namespace datagen::generator {

/// @brief Register business-related generators.
void register_business_generators(GeneratorRegistry& registry);

}  // namespace datagen::generator

#endif  // DATAGEN_BUSINESS_GENERATORS_H
