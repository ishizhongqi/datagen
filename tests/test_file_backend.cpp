/// @file test_file_backend.cpp

#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "config/configuration.h"
#include "engine/executor.h"
#include "output/file_backend.h"

using data_generator::config::GenerationConfig;
using data_generator::config::ParseMode;
using data_generator::config::ValidationIssue;

namespace {

GenerationConfig parse_or_fail(const nlohmann::json& root) {
    GenerationConfig cfg;
    std::vector<ValidationIssue> issues;
    const bool ok = data_generator::config::parse_generation_config(
        root,
        ParseMode::RequireOutputSettings,
        &cfg,
        &issues
    );
    EXPECT_TRUE(ok);
    EXPECT_TRUE(issues.empty());
    return cfg;
}

nlohmann::json base_fields() {
    return nlohmann::json::array({
        {
            {"name", "id"},
            {"generator", "integer"},
            {"config", {{"start", 1}, {"end", 9}}}
        },
        {
            {"name", "uid"},
            {"generator", "uuid"},
            {"config", {{"include_hyphens", true}}}
        }
    });
}

void write_small_file(const GenerationConfig& cfg, const std::filesystem::path& path) {
    data_generator::output::FileBackend backend;
    data_generator::engine::ExecutionOptions options;

    GenerationConfig with_path = cfg;
    with_path.output.file.path = path.string();

    const auto stats = backend.generate(with_path, options);
    EXPECT_EQ(stats.rows_generated, static_cast<std::uint64_t>(with_path.rows));
    EXPECT_EQ(stats.rows_written, static_cast<std::uint64_t>(with_path.rows));
    EXPECT_TRUE(std::filesystem::exists(path));
}

}  // namespace

TEST(FileBackendTest, WritesCsvJsonSqlAndDelimitedFormats) {
    const auto temp_dir = std::filesystem::temp_directory_path();

    nlohmann::json csv_root = {
        {"rows", 3},
        {"output", {
            {"type", "file"},
            {"file", {
                {"format", "csv"},
                {"options", {{"header", true}, {"line_ending", "LF"}}}
            }}
        }},
        {"fields", base_fields()}
    };
    auto csv_cfg = parse_or_fail(csv_root);
    write_small_file(csv_cfg, temp_dir / "dg_csv.csv");

    nlohmann::json tab_root = csv_root;
    tab_root["output"]["file"]["format"] = "Tab-Delimited";
    tab_root["output"]["file"]["options"] = {{"header", true}, {"line_ending", "CRLF"}};
    auto tab_cfg = parse_or_fail(tab_root);
    write_small_file(tab_cfg, temp_dir / "dg_tab.tsv");

    nlohmann::json custom_root = csv_root;
    custom_root["output"]["file"]["format"] = "Custom";
    custom_root["output"]["file"]["options"] = {
        {"delimiter", "|"},
        {"quote", "'"},
        {"header", true},
        {"line_ending", "LF"}
    };
    auto custom_cfg = parse_or_fail(custom_root);
    write_small_file(custom_cfg, temp_dir / "dg_custom.txt");

    nlohmann::json json_root = csv_root;
    json_root["output"]["file"]["format"] = "json";
    json_root["output"]["file"]["options"] = {{"array", true}, {"include_null", false}};
    auto json_cfg = parse_or_fail(json_root);
    write_small_file(json_cfg, temp_dir / "dg_data.json");

    nlohmann::json sql_root = csv_root;
    sql_root["output"]["file"]["format"] = "sql";
    sql_root["output"]["file"]["options"] = {{"table", "t_data"}, {"create_table", true}};
    auto sql_cfg = parse_or_fail(sql_root);
    write_small_file(sql_cfg, temp_dir / "dg_data.sql");
}

TEST(FileBackendTest, CancelsLargeOutputPrompt) {
    nlohmann::json root = {
        {"rows", 50000000},
        {"output", {
            {"type", "file"},
            {"file", {
                {"format", "csv"},
                {"options", {{"header", false}, {"line_ending", "LF"}}}
            }}
        }},
        {"fields", base_fields()}
    };

    auto cfg = parse_or_fail(root);
    cfg.output.file.path = (std::filesystem::temp_directory_path() / "dg_large.csv").string();

    std::istringstream input("n\n");
    auto* old = std::cin.rdbuf(input.rdbuf());

    data_generator::output::FileBackend backend;
    data_generator::engine::ExecutionOptions options;

    EXPECT_THROW(backend.generate(cfg, options), std::runtime_error);

    std::cin.rdbuf(old);
}

TEST(FileBackendTest, FailsWhenOutputPathIsNotWritable) {
    const auto base_dir = std::filesystem::temp_directory_path() / "dg_unwritable_dir";
    std::error_code ec;
    std::filesystem::create_directories(base_dir, ec);

    std::filesystem::permissions(
        base_dir,
        std::filesystem::perms::owner_read | std::filesystem::perms::owner_exec,
        std::filesystem::perm_options::replace,
        ec
    );

    nlohmann::json root = {
        {"rows", 2},
        {"output", {
            {"type", "file"},
            {"file", {
                {"format", "csv"},
                {"options", {{"header", true}, {"line_ending", "LF"}}}
            }}
        }},
        {"fields", base_fields()}
    };

    auto cfg = parse_or_fail(root);
    cfg.output.file.path = (base_dir / "output.csv").string();

    data_generator::output::FileBackend backend;
    data_generator::engine::ExecutionOptions options;

    EXPECT_THROW(backend.generate(cfg, options), std::runtime_error);

    std::filesystem::permissions(
        base_dir,
        std::filesystem::perms::owner_all,
        std::filesystem::perm_options::replace,
        ec
    );
    std::filesystem::remove_all(base_dir, ec);
}
