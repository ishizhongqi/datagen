/// @file test_file_backend.cpp

#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "config/configuration.h"
#include "engine/executor.h"
#include "output/file_backend.h"
#include "test_paths.h"

using datagen::config::GenerationConfig;
using datagen::config::ParseMode;
using datagen::config::ValidationIssue;

namespace {

GenerationConfig parse_or_fail(const nlohmann::json& root) {
    GenerationConfig cfg;
    std::vector<ValidationIssue> issues;
    const bool ok = datagen::config::parse_generation_config(
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
    datagen::output::FileBackend backend;
    datagen::engine::ExecutionOptions options;

    GenerationConfig with_path = cfg;
    with_path.output.file.path = path.string();

    const auto stats = backend.generate(with_path, options);
    EXPECT_EQ(stats.rows_generated, static_cast<std::uint64_t>(with_path.rows));
    EXPECT_EQ(stats.rows_written, static_cast<std::uint64_t>(with_path.rows));
    EXPECT_TRUE(std::filesystem::exists(path));
}

}  // namespace

TEST(FileBackendTest, WritesCsvJsonSqlAndDelimitedFormats) {
    const auto temp_dir = datagen::test::ensure_test_root();

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
    cfg.output.file.path = datagen::test::artifact_path("dg_large.csv").string();

    std::istringstream input("n\n");
    auto* old = std::cin.rdbuf(input.rdbuf());

    datagen::output::FileBackend backend;
    datagen::engine::ExecutionOptions options;

    EXPECT_THROW(backend.generate(cfg, options), std::runtime_error);

    std::cin.rdbuf(old);
}

TEST(FileBackendTest, FailsWhenOutputPathIsNotWritable) {
    const auto base_dir = datagen::test::artifact_path("dg_unwritable_dir");
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

    datagen::output::FileBackend backend;
    datagen::engine::ExecutionOptions options;

    EXPECT_THROW(backend.generate(cfg, options), std::runtime_error);

    std::filesystem::permissions(
        base_dir,
        std::filesystem::perms::owner_all,
        std::filesystem::perm_options::replace,
        ec
    );
    std::filesystem::remove_all(base_dir, ec);
}

TEST(FileBackendTest, UsesGeneratedDefaultOutputPathWhenEmpty) {
    const auto base_dir = datagen::test::artifact_path("dg_file_backend_default");
    std::error_code ec;
    std::filesystem::remove_all(base_dir, ec);
    std::filesystem::create_directories(base_dir, ec);

    nlohmann::json root = {
        {"rows", 1},
        {"output", {
            {"type", "file"},
            {"file", {
                {"format", "json"},
                {"options", {{"array", false}, {"include_null", true}}}
            }}
        }},
        {"fields", base_fields()}
    };

    auto cfg = parse_or_fail(root);
    cfg.output.file.path.clear();

    const auto original_cwd = std::filesystem::current_path();
    std::filesystem::current_path(base_dir, ec);
    ASSERT_FALSE(ec);

    datagen::output::FileBackend backend;
    datagen::engine::ExecutionOptions options;

    const auto stats = backend.generate(cfg, options);
    EXPECT_EQ(stats.rows_generated, 1U);

    std::filesystem::current_path(original_cwd, ec);
    ASSERT_FALSE(ec);

    std::size_t generated_file_count = 0;
    for (const auto& entry : std::filesystem::directory_iterator(base_dir)) {
        if (!entry.is_regular_file()) { continue; }
        const std::string filename = entry.path().filename().string();
        if (filename.rfind("dgresult_", 0) == 0 && entry.path().extension() == ".json") {
            ++generated_file_count;
        }
    }
    EXPECT_EQ(generated_file_count, 1U);

    std::filesystem::remove_all(base_dir, ec);
}

TEST(FileBackendTest, UsesExpectedDefaultExtensionForEachOutputFormat) {
    struct FormatCase {
        std::string    format;
        nlohmann::json options;
        std::string    expected_extension;
    };

    const std::vector<FormatCase> cases = {
        {"csv", {{"header", true}, {"line_ending", "LF"}}, ".csv"},
        {"json", {{"array", true}, {"include_null", true}}, ".json"},
        {"sql", {{"table", "t_data"}, {"create_table", true}}, ".sql"},
        {"Tab-Delimited", {{"header", true}, {"line_ending", "CRLF"}}, ".tsv"},
        {"Custom", {{"delimiter", "|"}, {"quote", "'"}, {"header", true}, {"line_ending", "LF"}}, ".txt"},
    };

    datagen::output::FileBackend backend;
    datagen::engine::ExecutionOptions options;
    const auto original_cwd = std::filesystem::current_path();

    for (const auto& format_case : cases) {
        const auto base_dir =
            datagen::test::artifact_path("dg_default_ext_" + format_case.expected_extension.substr(1));
        std::error_code ec;
        std::filesystem::remove_all(base_dir, ec);
        std::filesystem::create_directories(base_dir, ec);
        ASSERT_FALSE(ec);

        nlohmann::json root = {
            {"rows", 1},
            {"output", {
                {"type", "file"},
                {"file", {
                    {"format", format_case.format},
                    {"options", format_case.options}
                }}
            }},
            {"fields", base_fields()}
        };

        auto cfg = parse_or_fail(root);
        cfg.output.file.path.clear();

        std::filesystem::current_path(base_dir, ec);
        ASSERT_FALSE(ec);
        const auto stats = backend.generate(cfg, options);
        EXPECT_EQ(stats.rows_generated, 1U);

        std::filesystem::current_path(original_cwd, ec);
        ASSERT_FALSE(ec);

        std::size_t generated_file_count = 0;
        for (const auto& entry : std::filesystem::directory_iterator(base_dir)) {
            if (!entry.is_regular_file()) { continue; }
            const std::string filename = entry.path().filename().string();
            if (filename.rfind("dgresult_", 0) != 0) { continue; }
            EXPECT_EQ(entry.path().extension(), format_case.expected_extension);
            ++generated_file_count;
        }
        EXPECT_EQ(generated_file_count, 1U);

        std::filesystem::remove_all(base_dir, ec);
    }
}

TEST(FileBackendTest, EstimatesNullValuesWhenSizingRows) {
    const auto output_path = datagen::test::artifact_path("dg_null_row_estimate.json");
    std::error_code ec;
    std::filesystem::remove(output_path, ec);

    nlohmann::json root = {
        {"rows", 1},
        {"output", {
            {"type", "file"},
            {"file", {
                {"format", "json"},
                {"options", {{"array", true}, {"include_null", true}}}
            }}
        }},
        {"fields", nlohmann::json::array({
            {
                {"name", "nullable_value"},
                {"generator", "regular_expression"},
                {"config", {{"pattern", "x"}}},
                {"null_value", {{"enabled", true}, {"percentage", 100}}}
            }
        })}
    };

    auto cfg = parse_or_fail(root);
    cfg.output.file.path = output_path.string();

    datagen::output::FileBackend backend;
    datagen::engine::ExecutionOptions options;

    const auto stats = backend.generate(cfg, options);
    EXPECT_EQ(stats.rows_generated, 1U);

    std::ifstream input(output_path);
    ASSERT_TRUE(input.is_open());
    std::stringstream buffer;
    buffer << input.rdbuf();
    EXPECT_NE(buffer.str().find("null"), std::string::npos);

    std::filesystem::remove(output_path, ec);
}

TEST(FileBackendTest, PrintsProgressWhenGeneratedRowsReachTwoThousand) {
    const auto output_path = datagen::test::artifact_path("dg_progress_rows.csv");
    std::error_code ec;
    std::filesystem::remove(output_path, ec);

    nlohmann::json root = {
        {"rows", 2000},
        {"output", {
            {"type", "file"},
            {"file", {
                {"format", "csv"},
                {"options", {{"header", true}, {"line_ending", "LF"}}}
            }}
        }},
        {"fields", nlohmann::json::array({
            {
                {"name", "id"},
                {"generator", "integer"},
                {"config", {{"start", 1}, {"end", 5000}}}
            }
        })}
    };

    auto cfg = parse_or_fail(root);
    cfg.output.file.path = output_path.string();

    std::ostringstream captured_output;
    auto* old_buffer = std::cout.rdbuf(captured_output.rdbuf());

    datagen::output::FileBackend backend;
    datagen::engine::ExecutionOptions options;
    const auto stats = backend.generate(cfg, options);

    std::cout.rdbuf(old_buffer);

    EXPECT_EQ(stats.rows_generated, 2000U);
    EXPECT_TRUE(std::filesystem::exists(output_path));
    EXPECT_NE(captured_output.str().find("[Rows Generated]"), std::string::npos);

    std::filesystem::remove(output_path, ec);
}
