// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file payment_generators.h

#ifndef DATAGEN_PAYMENT_GENERATORS_H
#define DATAGEN_PAYMENT_GENERATORS_H

#include "generators/core/generator_registry.h"

namespace datagen::generator {

/// @brief Register payment-related generators.
void register_payment_generators(GeneratorRegistry& registry);

}  // namespace datagen::generator

#endif  // DATAGEN_PAYMENT_GENERATORS_H
