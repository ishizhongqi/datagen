// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file executor.h

#ifndef DATA_GENERATOR_EXECUTOR_H
#define DATA_GENERATOR_EXECUTOR_H

#include <cstdint>
#include <functional>
#include <optional>
#include <ostream>
#include <string>
#include <vector>

#include "config/configuration.h"

namespace data_generator::engine {

struct ExecutionOptions {
    std::size_t requested_threads = 1;
};

struct ExecutionInfo {
    std::size_t threads_used              = 1;
    bool        fallback_to_single_thread = false;
    std::string fallback_reason;
};

struct GenerateResult {
    ExecutionInfo  info;
    std::uint64_t  rows_generated = 0;
};

using Row = std::vector<std::optional<std::string>>;

using RowConsumer = std::function<bool(Row&& row, std::uint64_t row_index)>;

GenerateResult generate_with_consumer(
    const config::GenerationConfig& cfg,
    const ExecutionOptions& opts,
    const RowConsumer&      consumer
);

GenerateResult generate_to_stream(
    const config::GenerationConfig& cfg,
    const ExecutionOptions&         opts,
    std::ostream&                   out
);

std::vector<std::optional<std::string>> preview_row(const config::GenerationConfig& cfg);

}  // namespace data_generator::engine

#endif
