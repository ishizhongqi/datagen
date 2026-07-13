// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file cli_dispatcher.h

#ifndef DATAGEN_CLI_DISPATCHER_H
#define DATAGEN_CLI_DISPATCHER_H

#include <string>
#include <vector>

namespace datagen::cli {

class CliDispatcher {
public:
    static int dispatch(const std::string& command, const std::vector<std::string>& args);
};

}  // namespace datagen::cli

#endif  // DATAGEN_CLI_DISPATCHER_H
