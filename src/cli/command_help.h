// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file command_help.h

#ifndef DATAGEN_COMMAND_HELP_H
#define DATAGEN_COMMAND_HELP_H

#include <string>
#include <vector>

namespace datagen::cli {

class CommandHelp {
public:
    static int run(const std::vector<std::string>& args);
};

}  // namespace datagen::cli

#endif  // DATAGEN_COMMAND_HELP_H
