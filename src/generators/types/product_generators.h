// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file product_generators.h

#ifndef DATA_GENERATOR_PRODUCT_GENERATORS_H
#define DATA_GENERATOR_PRODUCT_GENERATORS_H

#include "generators/core/generator_registry.h"

namespace data_generator::generator {

/// @brief Register product-related generators.
void register_product_generators(GeneratorRegistry& registry);

}  // namespace data_generator::generator

#endif  // DATA_GENERATOR_PRODUCT_GENERATORS_H
