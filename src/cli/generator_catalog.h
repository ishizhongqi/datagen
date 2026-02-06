// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file generator_catalog.h

#ifndef DATA_GENERATOR_CLI_GENERATOR_CATALOG_H
#define DATA_GENERATOR_CLI_GENERATOR_CATALOG_H

#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace data_generator::cli {

using Json = nlohmann::json;

struct ConfigParam {
    std::string name;
    std::string description;
    bool        required = false;
};

struct GeneratorMetadata {
    std::string              name;
    std::vector<ConfigParam> config_params;
    bool                     supports_unique       = false;
    bool                     supports_data_linkage = false;
    std::string              linkage_module;
    Json                     config_template;
};

const std::vector<GeneratorMetadata>& get_generator_catalog();

const GeneratorMetadata* find_generator_metadata(const std::string& name);

Json build_project_template();

}  // namespace data_generator::cli

#endif  // DATA_GENERATOR_CLI_GENERATOR_CATALOG_H
