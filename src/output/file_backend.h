// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file file_backend.h

#ifndef DATA_GENERATOR_FILE_BACKEND_H
#define DATA_GENERATOR_FILE_BACKEND_H

#include "output/output_backend.h"

namespace data_generator::output {

class FileBackend final : public IOutputBackend {
public:
    OutputStats generate(const config::GenerationConfig& cfg, const engine::ExecutionOptions& options) override;
};

}  // namespace data_generator::output

#endif
