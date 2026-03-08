// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file generator_catalog.h

#ifndef DATA_GENERATOR_GENERATOR_CATALOG_H
#define DATA_GENERATOR_GENERATOR_CATALOG_H

#include <nlohmann/json.hpp>
#include <string>
#include <utility>
#include <vector>

namespace data_generator::cli {

using Json = nlohmann::json;

/// @brief Metadata for a generator configuration field.
struct ConfigParam {
    std::string              name;
    std::string              type;
    std::string              description;
    bool                     required = false;
    std::vector<std::string> supported_values;

    ConfigParam() = default;

    ConfigParam(
        std::string              name_value,
        std::string              type_value,
        std::string              description_value,
        bool                     required_value = false,
        std::vector<std::string> supported_values_value = {}
    ) :
        name(std::move(name_value)),
        type(std::move(type_value)),
        description(std::move(description_value)),
        required(required_value),
        supported_values(std::move(supported_values_value)) {}
};

/// @brief Metadata describing a generator.
struct GeneratorMetadata {
    std::string              name;
    std::string              module;
    std::vector<ConfigParam> config_params;
    bool                     supports_unique       = false;
    bool                     supports_data_linkage = false;
    std::string              linkage_module;
    Json                     config_template;
};

/// @brief Return catalog of all generators.
const std::vector<GeneratorMetadata>& get_generator_catalog();

/// @brief Find generator metadata by name.
const GeneratorMetadata* find_generator_metadata(const std::string& name);

/// @brief Build a default project template.
Json build_project_template(int rows, const std::string& output_format);

}  // namespace data_generator::cli

#endif  // DATA_GENERATOR_CLI_GENERATOR_CATALOG_H
