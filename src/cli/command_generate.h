// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file command_generate.h

#ifndef DATA_GENERATOR_COMMAND_GENERATE_H
#define DATA_GENERATOR_COMMAND_GENERATE_H

#include <string>
#include <vector>

namespace data_generator::cli {

class CommandGenerate {
public:
    static int run(const std::vector<std::string>& args);
};

}  // namespace data_generator::cli

#endif  // DATA_GENERATOR_COMMAND_GENERATE_H
