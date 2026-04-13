// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file array_parser.h

#ifndef DATAGEN_ARRAY_PARSER_H
#define DATAGEN_ARRAY_PARSER_H

#include <faker/faker.h>

#include "generator_registry.h"

namespace datagen::generator {

std::vector<std::string> parse_string_array(const std::string& key, const Json& config);

faker::DaysOfWeek parse_days_of_week(const Json& config);

faker::Languages parse_languages(const Json& config);

faker::Regions parse_regions(const Json& config);

faker::Genders parse_genders(const Json& config);

faker::CardTypes parse_card_types(const Json& config);

faker::BarcodeTypes parse_barcode_type(const Json& config);

faker::OperatingSystems parse_operating_systems(const Json& config);

faker::CountryCodesStandard parse_country_codes_standard(const Json& config);

bool parse_use_translation(const Json& config);

}  // namespace datagen::generator

#endif  // DATAGEN_ARRAY_PARSER_H
