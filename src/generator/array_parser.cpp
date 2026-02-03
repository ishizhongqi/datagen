// Copyright (c) 2026 Shizhongqi
// Licensed under the Apache License 2.0.
// See the LICENSE file in the project root for more information.

/// @file array_parser.cpp

#include "array_parser.h"

#include <faker/faker.h>

#include "generator_registry.h"

namespace data_generator::generator {

std::vector<std::string> parse_string_array(const std::string& key, const Json& config) {
    if (!config.contains(key)) { return {}; }

    const auto&              ext_config = config[key];
    std::vector<std::string> result;

    if (ext_config.is_array()) {
        if (ext_config.empty()) { throw std::invalid_argument(key + " array must not be empty"); }

        for (const auto& ext : ext_config) {
            if (!ext.is_string()) { throw std::invalid_argument(key + " array elements must be strings"); }

            auto value = ext.get<std::string>();
            if (value.empty()) { throw std::invalid_argument(key + " must not be empty"); }

            result.emplace_back(std::move(value));
        }

        return result;
    }

    if (ext_config.is_string()) {
        auto value = ext_config.get<std::string>();
        if (value.empty()) { throw std::invalid_argument(key + " must not be empty"); }

        result.emplace_back(std::move(value));
        return result;
    }

    throw std::invalid_argument(key + " must be a string or an array of strings");
}

faker::DaysOfWeek parse_days_of_week(const Json& config) {
    if (!config.contains("days_of_week")) { return faker::DaysOfWeek::Monday; }
    const auto& days_config = config["days_of_week"];
    if (days_config.is_array()) {
        faker::DaysOfWeek days_of_week{};

        for (const auto& day : days_config) {
            const std::string day_str = day.get<std::string>();

            if (day_str == "Monday") {
                days_of_week |= faker::DaysOfWeek::Monday;
            } else if (day_str == "Tuesday") {
                days_of_week |= faker::DaysOfWeek::Tuesday;
            } else if (day_str == "Wednesday") {
                days_of_week |= faker::DaysOfWeek::Wednesday;
            } else if (day_str == "Thursday") {
                days_of_week |= faker::DaysOfWeek::Thursday;
            } else if (day_str == "Friday") {
                days_of_week |= faker::DaysOfWeek::Friday;
            } else if (day_str == "Saturday") {
                days_of_week |= faker::DaysOfWeek::Saturday;
            } else if (day_str == "Sunday") {
                days_of_week |= faker::DaysOfWeek::Sunday;
            } else {
                throw std::invalid_argument("Invalid day of week: " + day_str);
            }
        }

        if (days_of_week == faker::DaysOfWeek{}) {
            throw std::invalid_argument("days_of_week array must not be empty");
        }

        return days_of_week;
    }
    if (days_config.is_string()) {
        const std::string day_str = days_config.get<std::string>();
        if (day_str == "Monday") { return faker::DaysOfWeek::Monday; }
        if (day_str == "Tuesday") { return faker::DaysOfWeek::Tuesday; }
        if (day_str == "Wednesday") { return faker::DaysOfWeek::Wednesday; }
        if (day_str == "Thursday") { return faker::DaysOfWeek::Thursday; }
        if (day_str == "Friday") { return faker::DaysOfWeek::Friday; }
        if (day_str == "Saturday") { return faker::DaysOfWeek::Saturday; }
        if (day_str == "Sunday") { return faker::DaysOfWeek::Sunday; }
        throw std::invalid_argument("Invalid day of week: " + day_str);
    }

    throw std::invalid_argument("days_of_week must be a string or an array of strings");
}

faker::Languages parse_languages(const Json& config) {
    if (!config.contains("languages")) { return faker::Languages::English; }

    const auto& languages_config = config["languages"];

    if (languages_config.is_array()) {
        faker::Languages languages{};

        for (const auto& lang : languages_config) {
            const std::string language_str = lang.get<std::string>();

            if (language_str == "English") {
                languages |= faker::Languages::English;
            } else if (language_str == "Simplified Chinese" || language_str == "SimplifiedChinese") {
                languages |= faker::Languages::SimplifiedChinese;
            } else if (language_str == "Traditional Chinese" || language_str == "TraditionalChinese") {
                languages |= faker::Languages::TraditionalChinese;
            } else if (language_str == "Japanese") {
                languages |= faker::Languages::Japanese;
            } else {
                throw std::invalid_argument("Invalid language: " + language_str);
            }
        }

        if (languages == faker::Languages{}) { throw std::invalid_argument("languages array must not be empty"); }

        return languages;
    }
    if (languages_config.is_string()) {
        const std::string language_str = languages_config.get<std::string>();
        if (language_str == "English") { return faker::Languages::English; }
        if (language_str == "Simplified Chinese" || language_str == "SimplifiedChinese") {
            return faker::Languages::SimplifiedChinese;
        }
        if (language_str == "Traditional Chinese" || language_str == "TraditionalChinese") {
            return faker::Languages::TraditionalChinese;
        }
        if (language_str == "Japanese") { return faker::Languages::Japanese; }
        throw std::invalid_argument("Invalid language: " + language_str);
    }

    throw std::invalid_argument("languages must be a string or an array of strings");
}

faker::Regions parse_regions(const Json& config) {
    if (!config.contains("regions")) { return faker::Regions::UnitedStates; }

    const auto& regions_config = config["regions"];

    if (regions_config.is_array()) {
        faker::Regions regions{};

        for (const auto& region : regions_config) {
            const std::string region_str = region.get<std::string>();

            if (region_str == "United States" || region_str == "UnitedStates") {
                regions |= faker::Regions::UnitedStates;
            } else if (region_str == "United Kingdom" || region_str == "UnitedKingdom") {
                regions |= faker::Regions::UnitedKingdom;
            } else if (region_str == "China") {
                regions |= faker::Regions::China;
            } else if (region_str == "Japan") {
                regions |= faker::Regions::Japan;
            } else {
                throw std::invalid_argument("Invalid region: " + region_str);
            }
        }

        if (regions == faker::Regions{}) { throw std::invalid_argument("regions array must not be empty"); }

        return regions;
    }
    if (regions_config.is_string()) {
        const std::string region_str = regions_config.get<std::string>();
        if (region_str == "United States" || region_str == "UnitedStates") { return faker::Regions::UnitedStates; }
        if (region_str == "United Kingdom" || region_str == "UnitedKingdom") {
            return faker::Regions::UnitedKingdom;
        }
        if (region_str == "China") { return faker::Regions::China; }
        if (region_str == "Japan") { return faker::Regions::Japan; }
        throw std::invalid_argument("Invalid region: " + region_str);
    }

    throw std::invalid_argument("regions must be a string or an array of strings");
}

faker::CardTypes parse_card_types(const Json& config) {
    if (!config.contains("card_types") && !config.contains("card_type")) { return faker::CardTypes::Visa; }

    const auto& card_types_config = config.contains("card_types") ? config["card_types"] : config["card_type"];

    if (card_types_config.is_array()) {
        faker::CardTypes card_types{};

        for (const auto& card_type : card_types_config) {
            const std::string card_type_str = card_type.get<std::string>();

            if (card_type_str == "American Express" || card_type_str == "AmericanExpress") {
                card_types |= faker::CardTypes::AmericanExpress;
            } else if (card_type_str == "JCB") {
                card_types |= faker::CardTypes::JCB;
            } else if (card_type_str == "Master Card" || card_type_str == "MasterCard") {
                card_types |= faker::CardTypes::MasterCard;
            } else if (card_type_str == "Union Pay" || card_type_str == "UnionPay") {
                card_types |= faker::CardTypes::UnionPay;
            } else if (card_type_str == "Visa") {
                card_types |= faker::CardTypes::Visa;
            } else {
                throw std::invalid_argument("Invalid card type: " + card_type_str);
            }
        }

        if (card_types == faker::CardTypes{}) { throw std::invalid_argument("card_types array must not be empty"); }

        return card_types;
    }

    if (card_types_config.is_string()) {
        const std::string card_type_str = card_types_config.get<std::string>();
        if (card_type_str == "American Express" || card_type_str == "AmericanExpress") {
            return faker::CardTypes::AmericanExpress;
        }
        if (card_type_str == "JCB") { return faker::CardTypes::JCB; }
        if (card_type_str == "Master Card" || card_type_str == "MasterCard") { return faker::CardTypes::MasterCard; }
        if (card_type_str == "Union Pay" || card_type_str == "UnionPay") { return faker::CardTypes::UnionPay; }
        if (card_type_str == "Visa") { return faker::CardTypes::Visa; }
        throw std::invalid_argument("Invalid card type: " + card_type_str);
    }

    throw std::invalid_argument("card_types must be a string or an array of strings");
}

faker::BarcodeTypes parse_barcode_type(const Json& config) {
    if (!config.contains("barcode_types") && !config.contains("barcode_type")) { return faker::BarcodeTypes::EAN13; }
    const auto& barcode_types_config = config.contains("barcode_types") ? config["barcode_types"] : config["barcode_type"];
    if (barcode_types_config.is_array()) {
        faker::BarcodeTypes barcode_types{};
        for (const auto& barcode_type : barcode_types_config) {
            const std::string barcode_type_str = barcode_type.get<std::string>();
            if (barcode_type_str == "EAN8") {
                barcode_types |= faker::BarcodeTypes::EAN8;
            } else if (barcode_type_str == "EAN13") {
                barcode_types |= faker::BarcodeTypes::EAN13;
            } else if (barcode_type_str == "UPCA") {
                barcode_types |= faker::BarcodeTypes::UPCA;
            } else if (barcode_type_str == "UPCE") {
                barcode_types |= faker::BarcodeTypes::UPCE;
            } else if (barcode_type_str == "ISBN") {
                barcode_types |= faker::BarcodeTypes::ISBN;
            } else {
                throw std::invalid_argument("Invalid barcode type: " + barcode_type_str);
            }
        }

        if (barcode_types == faker::BarcodeTypes{}) {
            throw std::invalid_argument("barcode_types array must not be empty");
        }

        return barcode_types;
    }

    if (barcode_types_config.is_string()) {
        const std::string barcode_type_str = barcode_types_config.get<std::string>();
        if (barcode_type_str == "EAN8") { return faker::BarcodeTypes::EAN8; }
        if (barcode_type_str == "EAN13") { return faker::BarcodeTypes::EAN13; }
        if (barcode_type_str == "UPCA") { return faker::BarcodeTypes::UPCA; }
        if (barcode_type_str == "UPCE") { return faker::BarcodeTypes::UPCE; }
        if (barcode_type_str == "ISBN") { return faker::BarcodeTypes::ISBN; }
        throw std::invalid_argument("Invalid barcode type: " + barcode_type_str);
    }

    throw std::invalid_argument("barcode_types must be a string or an array of strings");
}

faker::OperatingSystems parse_operating_systems(const Json& config) {
    if (!config.contains("operating_systems")) { return faker::OperatingSystems::Windows; }

    const auto& os_config = config["operating_systems"];

    if (os_config.is_array()) {
        faker::OperatingSystems operating_systems{};

        for (const auto& os : os_config) {
            const std::string operating_system_str = os.get<std::string>();

            if (operating_system_str == "Windows") {
                operating_systems |= faker::OperatingSystems::Windows;
            } else if (operating_system_str == "macOS") {
                operating_systems |= faker::OperatingSystems::macOS;
            } else if (operating_system_str == "Linux") {
                operating_systems |= faker::OperatingSystems::Linux;
            } else {
                throw std::invalid_argument("Invalid operating system: " + operating_system_str);
            }
        }

        if (operating_systems == faker::OperatingSystems{}) {
            throw std::invalid_argument("operating_systems array must not be empty");
        }

        return operating_systems;
    }
    if (os_config.is_string()) {
        const std::string os_str = os_config.get<std::string>();
        if (os_str == "Windows") { return faker::OperatingSystems::Windows; }
        if (os_str == "macOS") { return faker::OperatingSystems::macOS; }
        if (os_str == "Linux") { return faker::OperatingSystems::Linux; }
        throw std::invalid_argument("Invalid operating system: " + os_str);
    }

    throw std::invalid_argument("operating_systems must be a string or an array of strings");
}

}  // namespace data_generator::generator
