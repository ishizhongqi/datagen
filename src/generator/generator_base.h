// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file generator_base.h

#ifndef DATA_GENERATOR_GENERATOR_BASE_H
#define DATA_GENERATOR_GENERATOR_BASE_H

#include <string>

namespace data_generator::generator {

/// @brief Base interface for all generators.
///        One generator instance corresponds to one column behavior.
class IGenerator {
public:
    virtual ~IGenerator() = default;

    /// @brief Generate value for current row.
    /// @return Generated string value.
    virtual std::string generate() = 0;

    /// @brief Called when moving to next row.
    ///        Stateful generators (e.g. File) should reroll here.
    virtual void next() {}
};

}  // namespace data_generator::generator

#endif  // DATA_GENERATOR_GENERATOR_BASE_H
