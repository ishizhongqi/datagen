// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file exit_codes.h

#ifndef DATA_GENERATOR_EXIT_CODES_H
#define DATA_GENERATOR_EXIT_CODES_H

namespace data_generator::cli::exit_codes {

constexpr int kOk             = 0;
constexpr int kCliError       = 1;
constexpr int kUsage          = 2;
constexpr int kRuntimeFailure = 3;

}  // namespace data_generator::cli::exit_codes

#endif
