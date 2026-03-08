// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file output_backend.h

#ifndef DATA_GENERATOR_OUTPUT_BACKEND_H
#define DATA_GENERATOR_OUTPUT_BACKEND_H

#include <cstdint>
#include <memory>

#include "core/configuration.h"
#include "core/executor.h"

namespace data_generator::output {

struct OutputStats {
    core::ExecutionInfo execution_info;
    std::uint64_t       rows_generated = 0;
    std::uint64_t       rows_written = 0;
};

class IOutputBackend {
public:
    virtual ~IOutputBackend() = default;

    virtual OutputStats generate(const core::GenerationConfig& cfg, const core::ExecutionOptions& options) = 0;
};

std::unique_ptr<IOutputBackend> make_output_backend(const core::GenerationConfig& cfg);

}  // namespace data_generator::output

#endif
