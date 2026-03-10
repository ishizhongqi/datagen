// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file progress_bar.h

#ifndef DATA_GENERATOR_PROGRESS_BAR_H
#define DATA_GENERATOR_PROGRESS_BAR_H

#include <cstdint>
#include <string>

namespace data_generator::core {

std::string format_progress_bar(std::uint64_t done, std::uint64_t total);

}  // namespace data_generator::core

#endif
