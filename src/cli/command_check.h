// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file command_check.h

#ifndef DATA_GENERATOR_COMMAND_CHECK_H
#define DATA_GENERATOR_COMMAND_CHECK_H

#include <string>
#include <vector>

namespace data_generator::cli {

class CommandCheck {
public:
    static int run(const std::vector<std::string>& args);
};

}  // namespace data_generator::cli

#endif  // DATA_GENERATOR_COMMAND_CHECK_H
