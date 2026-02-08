// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file command_list.h

#ifndef DATA_GENERATOR_COMMAND_LIST_H
#define DATA_GENERATOR_COMMAND_LIST_H

#include <string>
#include <vector>

namespace data_generator::cli {

/// @brief CLI command to list available generators.
class CommandList {
public:
    /// @brief Execute the list command.
    static int run(const std::vector<std::string>& args);
};

}  // namespace data_generator::cli

#endif  // DATA_GENERATOR_COMMAND_LIST_H
