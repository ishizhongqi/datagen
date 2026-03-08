// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file database_backend.h

#ifndef DATA_GENERATOR_DATABASE_BACKEND_H
#define DATA_GENERATOR_DATABASE_BACKEND_H

#include "output/output_backend.h"

namespace data_generator::output {

class DatabaseBackend final : public IOutputBackend {
public:
    OutputStats generate(const core::GenerationConfig& cfg, const core::ExecutionOptions& options) override;
};

}  // namespace data_generator::output

#endif
