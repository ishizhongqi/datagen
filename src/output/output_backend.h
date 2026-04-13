// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file output_backend.h

#ifndef DATAGEN_OUTPUT_BACKEND_H
#define DATAGEN_OUTPUT_BACKEND_H

#include <cstdint>
#include <memory>

#include "config/configuration.h"
#include "engine/executor.h"

namespace datagen::output {

struct OutputStats {
    engine::ExecutionInfo execution_info;
    std::uint64_t         rows_generated = 0;
    std::uint64_t         rows_written   = 0;
};

class IOutputBackend {
public:
    virtual ~IOutputBackend() = default;

    virtual OutputStats generate(const config::GenerationConfig& cfg, const engine::ExecutionOptions& options) = 0;
};

std::unique_ptr<IOutputBackend> make_output_backend(const config::GenerationConfig& cfg);

}  // namespace datagen::output

#endif
