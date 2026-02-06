// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file cli_dispatcher.h

#ifndef DATA_GENERATOR_CLI_DISPATCHER_H
#define DATA_GENERATOR_CLI_DISPATCHER_H

#include <string>
#include <vector>

namespace data_generator::cli {

class CliDispatcher {
public:
    static int dispatch(const std::string& command, const std::vector<std::string>& args);
};

}  // namespace data_generator::cli

#endif  // DATA_GENERATOR_CLI_DISPATCHER_H
