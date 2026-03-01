// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

#ifndef DATA_GENERATOR_DATABASE_TYPE_ADAPTER_H
#define DATA_GENERATOR_DATABASE_TYPE_ADAPTER_H

#include <optional>
#include <string>

#include "database/db_metadata.h"

namespace data_generator::database {

struct AdaptedValue {
    bool        ok = false;
    bool        is_null = false;
    std::string converted_value;
    std::string sql_literal;
    std::string error_type;
    std::string error_message;
};

class ITypeAdapter {
public:
    virtual ~ITypeAdapter() = default;

    virtual AdaptedValue adapt(
        const ColumnMetadata&            column,
        const std::optional<std::string>& raw_value
    ) const = 0;
};

class DefaultTypeAdapter final : public ITypeAdapter {
public:
    AdaptedValue adapt(const ColumnMetadata& column, const std::optional<std::string>& raw_value) const override;
};

}  // namespace data_generator::database

#endif
