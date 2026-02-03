// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file string_generators.h

#ifndef DATA_GENERATOR_STRING_GENERATORS_H
#define DATA_GENERATOR_STRING_GENERATORS_H

#include "generator_registry.h"

namespace data_generator::generator {

/// @brief Register string-related generators.
void register_string_generators(GeneratorRegistry& registry);

}  // namespace data_generator::generator

#endif  // DATA_GENERATOR_STRING_GENERATORS_H
