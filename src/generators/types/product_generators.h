// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file product_generators.h

#ifndef DATAGEN_PRODUCT_GENERATORS_H
#define DATAGEN_PRODUCT_GENERATORS_H

#include "generators/core/generator_registry.h"

namespace datagen::generator {

/// @brief Register product-related generators.
void register_product_generators(GeneratorRegistry& registry);

}  // namespace datagen::generator

#endif  // DATAGEN_PRODUCT_GENERATORS_H
