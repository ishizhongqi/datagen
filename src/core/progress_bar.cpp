// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file progress_bar.cpp

#include "core/progress_bar.h"

#include <algorithm>
#include <sstream>

namespace data_generator::core {

std::string format_progress_bar(const std::uint64_t done, const std::uint64_t total) {
    constexpr int kWidth = 20;
    const double progress = (total == 0) ? 1.0 : static_cast<double>(done) / static_cast<double>(total);
    const int filled = static_cast<int>(progress * static_cast<double>(kWidth));

    std::string bar;
    bar.reserve(kWidth);
    for (int i = 0; i < kWidth; ++i) {
        bar.push_back(i < filled ? '#' : '-');
    }

    std::ostringstream oss;
    oss << "[" << bar << "] " << static_cast<int>(progress * 100.0) << "% (" << done << "/" << total << ")";
    return oss.str();
}

}  // namespace data_generator::core
