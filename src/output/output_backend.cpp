// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file output_backend.cpp

#include "output/output_backend.h"

#include "output/database_backend.h"
#include "output/file_backend.h"

namespace data_generator::output {

std::unique_ptr<IOutputBackend> make_output_backend(const core::GenerationConfig& cfg) {
    if (cfg.output.destination == core::OutputDestination::Database) {
        return std::make_unique<DatabaseBackend>();
    }
    return std::make_unique<FileBackend>();
}

}  // namespace data_generator::output
