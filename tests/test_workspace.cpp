/// @file test_workspace.cpp

#include <gtest/gtest.h>

#include <filesystem>
#include <optional>
#include <string>

#include "utils/workspace.h"

using data_generator::utils::default_workspace_root;
using data_generator::utils::ensure_workspace_layout;
using data_generator::utils::is_workspace_local_filename;
using data_generator::utils::resolve_workspace_root;

TEST(WorkspaceTest, ResolvesWorkspacePaths) {
    const auto root = default_workspace_root();
    EXPECT_FALSE(root.empty());

#if !defined(_WIN32)
    EXPECT_NE(root.string().find(".data_generator"), std::string::npos);
#endif

    const std::optional<std::string> empty;
    const auto resolved_default = resolve_workspace_root(empty);
    EXPECT_EQ(resolved_default, root);

    const auto relative = resolve_workspace_root(std::optional<std::string>("child"));
    EXPECT_EQ(relative, root / "child");

    const auto absolute = resolve_workspace_root(std::optional<std::string>("/tmp/dg_workspace"));
    EXPECT_EQ(absolute, std::filesystem::path("/tmp/dg_workspace"));
}

TEST(WorkspaceTest, ValidatesLocalFilename) {
    EXPECT_FALSE(is_workspace_local_filename(""));
    EXPECT_FALSE(is_workspace_local_filename("/tmp/abs.txt"));
    EXPECT_FALSE(is_workspace_local_filename("dir/file.txt"));
    EXPECT_TRUE(is_workspace_local_filename("file.txt"));
}

TEST(WorkspaceTest, EnsuresWorkspaceLayoutCreatesDirectories) {
    const auto base = std::filesystem::temp_directory_path() / "dg_workspace_test";
    std::error_code ec;
    std::filesystem::remove_all(base, ec);

    std::string error;
    const auto layout = ensure_workspace_layout(std::optional<std::string>(base.string()), &error);
    EXPECT_TRUE(error.empty());
    EXPECT_EQ(layout.root, base);
    EXPECT_TRUE(std::filesystem::exists(layout.log_dir));
    EXPECT_TRUE(std::filesystem::exists(layout.tmp_dir));
    EXPECT_TRUE(std::filesystem::exists(layout.ui_dir));

    std::filesystem::remove_all(base, ec);
}
