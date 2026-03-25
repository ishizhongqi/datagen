/// @file test_paths.h

#ifndef DATA_GENERATOR_TEST_PATHS_H
#define DATA_GENERATOR_TEST_PATHS_H

#include <filesystem>
#include <string>

#include "utils/workspace.h"

namespace data_generator::test {

inline std::filesystem::path test_root() {
    return utils::default_workspace_root() / "test_artifacts";
}

inline std::filesystem::path ensure_test_root() {
    const std::filesystem::path root = test_root();
    std::error_code ec;
    std::filesystem::create_directories(root, ec);
    return root;
}

inline std::filesystem::path artifact_path(const std::string& name) {
    return ensure_test_root() / name;
}

inline std::filesystem::path workspace_path(const std::string& name) {
    const std::filesystem::path root = ensure_test_root() / "workspaces" / name;
    std::error_code ec;
    std::filesystem::create_directories(root, ec);
    return root;
}

inline void reset_path(const std::filesystem::path& path) {
    std::error_code ec;
    std::filesystem::remove_all(path, ec);
    const std::filesystem::path parent = path.parent_path();
    if (!parent.empty()) {
        std::filesystem::create_directories(parent, ec);
    }
}

}  // namespace data_generator::test

#endif
