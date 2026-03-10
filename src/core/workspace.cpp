// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file workspace.cpp

#include "core/workspace.h"

#include <optional>
#include <string>

#include "core/env_utils.h"

namespace data_generator::core {

namespace {

std::filesystem::path home_directory() {
#if defined(_WIN32)
    if (const auto user_profile = get_env_value("USERPROFILE"); user_profile.has_value()) {
        return std::filesystem::path(*user_profile);
    }
    if (const auto home = get_env_value("HOME"); home.has_value()) {
        return std::filesystem::path(*home);
    }
#else
    if (const auto home = get_env_value("HOME"); home.has_value()) {
        return std::filesystem::path(*home);
    }
#endif
    return std::filesystem::current_path();
}

}  // namespace

std::filesystem::path default_workspace_root() {
#if defined(_WIN32)
    return home_directory() / "Documents" / "DataGenerator";
#else
    return home_directory() / ".data_generator";
#endif
}

std::filesystem::path resolve_workspace_root(const std::optional<std::string>& configured_workspace) {
    if (!configured_workspace.has_value() || configured_workspace->empty()) {
        return default_workspace_root();
    }

    std::filesystem::path path = *configured_workspace;
    if (path.is_relative()) {
        path = default_workspace_root() / path;
    }
    return path;
}

bool is_workspace_local_filename(const std::string& filename) {
    if (filename.empty()) { return false; }
    const std::filesystem::path path(filename);
    if (path.is_absolute()) { return false; }
    if (path.has_parent_path()) { return false; }
    return true;
}

WorkspaceLayout ensure_workspace_layout(
    const std::optional<std::string>& configured_workspace,
    std::string*                      error_message
) {
    WorkspaceLayout layout;
    layout.root = resolve_workspace_root(configured_workspace);
    layout.log_dir = layout.root / "log";
    layout.tmp_dir = layout.root / "tmp";
    layout.ui_dir = layout.root / "ui";
    layout.generate_log_file = layout.log_dir / "generate.log";
    layout.error_log_file = layout.log_dir / "error.log";

    std::error_code ec;
    std::filesystem::create_directories(layout.log_dir, ec);
    if (ec) {
        if (error_message) { *error_message = "failed to create log directory: " + ec.message(); }
        return WorkspaceLayout{};
    }

    std::filesystem::create_directories(layout.tmp_dir, ec);
    if (ec) {
        if (error_message) { *error_message = "failed to create tmp directory: " + ec.message(); }
        return WorkspaceLayout{};
    }

    std::filesystem::create_directories(layout.ui_dir, ec);
    if (ec) {
        if (error_message) { *error_message = "failed to create ui directory: " + ec.message(); }
        return WorkspaceLayout{};
    }

    return layout;
}

}  // namespace data_generator::core
