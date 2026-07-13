// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file database_backend.h

#ifndef DATAGEN_DATABASE_BACKEND_H
#define DATAGEN_DATABASE_BACKEND_H

#include "output/output_backend.h"

namespace datagen::output {

class DatabaseBackend final : public IOutputBackend {
public:
    OutputStats generate(const config::GenerationConfig& cfg, const engine::ExecutionOptions& options) override;
};

}  // namespace datagen::output

#endif
