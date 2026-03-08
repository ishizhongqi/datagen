// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file workspace.h

#ifndef DATA_GENERATOR_WORKSPACE_H
#define DATA_GENERATOR_WORKSPACE_H

#include <filesystem>
#include <optional>
#include <string>

namespace data_generator::core {

struct WorkspaceLayout {
    std::filesystem::path root;
    std::filesystem::path log_dir;
    std::filesystem::path tmp_dir;
    std::filesystem::path ui_dir;
    std::filesystem::path generate_log_file;
    std::filesystem::path error_log_file;
};

std::filesystem::path default_workspace_root();

std::filesystem::path resolve_workspace_root(const std::optional<std::string>& configured_workspace);

bool is_workspace_local_filename(const std::string& filename);

WorkspaceLayout ensure_workspace_layout(
    const std::optional<std::string>& configured_workspace,
    std::string*                      error_message
);

}  // namespace data_generator::core

#endif
