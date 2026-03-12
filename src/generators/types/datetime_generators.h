// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file datetime_generators.h

#ifndef DATA_GENERATOR_DATETIME_GENERATORS_H
#define DATA_GENERATOR_DATETIME_GENERATORS_H

#include "generators/core/generator_registry.h"

namespace data_generator::generator {

/// @brief Register datetime-related generators.
void register_datetime_generators(GeneratorRegistry& registry);

}  // namespace data_generator::generator

#endif  // DATA_GENERATOR_DATETIME_GENERATORS_H
