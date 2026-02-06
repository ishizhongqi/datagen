// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file number_generators.h

#ifndef DATA_GENERATOR_NUMBER_GENERATORS_H
#define DATA_GENERATOR_NUMBER_GENERATORS_H

#include "generator_registry.h"

namespace data_generator::generator {

/// @brief Register number-related generators.
void register_number_generators(GeneratorRegistry& registry);

}  // namespace data_generator::generator

#endif  // DATA_GENERATOR_NUMBER_GENERATORS_H
