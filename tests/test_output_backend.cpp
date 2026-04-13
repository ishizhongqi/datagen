/// @file test_output_backend.cpp

#include <gtest/gtest.h>

#include "config/configuration.h"
#include "output/database_backend.h"
#include "output/file_backend.h"
#include "output/output_backend.h"

using datagen::config::GenerationConfig;
using datagen::output::DatabaseBackend;
using datagen::output::FileBackend;
using datagen::output::make_output_backend;

TEST(OutputBackendTest, ReturnsExpectedBackendType) {
    GenerationConfig cfg;
    cfg.output.type = datagen::config::OutputType::File;
    auto file_backend = make_output_backend(cfg);
    EXPECT_NE(dynamic_cast<FileBackend*>(file_backend.get()), nullptr);

    cfg.output.type = datagen::config::OutputType::Database;
    auto db_backend = make_output_backend(cfg);
    EXPECT_NE(dynamic_cast<DatabaseBackend*>(db_backend.get()), nullptr);
}
