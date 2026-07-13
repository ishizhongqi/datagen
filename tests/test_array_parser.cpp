/// @file test_array_parser.cpp

#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include <vector>

#include "generators/core/array_parser.h"

using namespace datagen::generator;

namespace {

TEST(ArrayParserTest, ParseStringArrayVariants) {
    const nlohmann::json empty = nlohmann::json::object();
    EXPECT_TRUE(parse_string_array("k", empty).empty());

    const nlohmann::json single = nlohmann::json::parse(R"json({"k":"v1"})json");
    EXPECT_EQ(parse_string_array("k", single), std::vector<std::string>({"v1"}));

    const nlohmann::json many = nlohmann::json::parse(R"json({"k":["a","b"]})json");
    EXPECT_EQ(parse_string_array("k", many), std::vector<std::string>({"a", "b"}));

    const nlohmann::json bad_empty = nlohmann::json::parse(R"json({"k":[]})json");
    EXPECT_THROW((void)parse_string_array("k", bad_empty), std::invalid_argument);

    const nlohmann::json bad_type = nlohmann::json::parse(R"json({"k":1})json");
    EXPECT_THROW((void)parse_string_array("k", bad_type), std::invalid_argument);

    const nlohmann::json bad_elem = nlohmann::json::parse(R"json({"k":[1]})json");
    EXPECT_THROW((void)parse_string_array("k", bad_elem), std::invalid_argument);
}

TEST(ArrayParserTest, ParseDaysOfWeekVariants) {
    const nlohmann::json default_cfg = nlohmann::json::object();
    EXPECT_NO_THROW((void)parse_days_of_week(default_cfg));

    const nlohmann::json one = nlohmann::json::parse(R"json({"days_of_week":"Monday"})json");
    EXPECT_NO_THROW((void)parse_days_of_week(one));

    const nlohmann::json many = nlohmann::json::parse(
        R"json({"days_of_week":["Monday","Tuesday","Wednesday","Thursday","Friday","Saturday","Sunday"]})json"
    );
    EXPECT_NO_THROW((void)parse_days_of_week(many));

    const nlohmann::json bad = nlohmann::json::parse(R"json({"days_of_week":"Xday"})json");
    EXPECT_THROW((void)parse_days_of_week(bad), std::invalid_argument);

    const nlohmann::json bad_arr = nlohmann::json::parse(R"json({"days_of_week":["Xday"]})json");
    EXPECT_THROW((void)parse_days_of_week(bad_arr), std::invalid_argument);

    const nlohmann::json bad_empty = nlohmann::json::parse(R"json({"days_of_week":[]})json");
    EXPECT_THROW((void)parse_days_of_week(bad_empty), std::invalid_argument);

    const nlohmann::json bad_type = nlohmann::json::parse(R"json({"days_of_week":1})json");
    EXPECT_THROW((void)parse_days_of_week(bad_type), std::invalid_argument);
}

TEST(ArrayParserTest, ParseLanguagesVariants) {
    EXPECT_NO_THROW((void)parse_languages(nlohmann::json::object()));
    EXPECT_NO_THROW((void)parse_languages(nlohmann::json::parse(R"json({"languages":"English"})json")));
    EXPECT_NO_THROW((void)parse_languages(nlohmann::json::parse(R"json({"languages":"Simplified Chinese"})json")));
    EXPECT_NO_THROW((void)parse_languages(nlohmann::json::parse(R"json({"languages":"TraditionalChinese"})json")));
    EXPECT_NO_THROW((void)parse_languages(nlohmann::json::parse(R"json({"languages":"Japanese"})json")));
    EXPECT_NO_THROW((void)parse_languages(
        nlohmann::json::parse(R"json({"languages":["English","SimplifiedChinese","Traditional Chinese","Japanese"]})json")
    ));
    EXPECT_THROW(
        (void)parse_languages(nlohmann::json::parse(R"json({"languages":"Klingon"})json")),
        std::invalid_argument
    );
    EXPECT_THROW(
        (void)parse_languages(nlohmann::json::parse(R"json({"languages":["Klingon"]})json")),
        std::invalid_argument
    );
    EXPECT_THROW((void)parse_languages(nlohmann::json::parse(R"json({"languages":1})json")), std::invalid_argument);
}

TEST(ArrayParserTest, ParseRegionsVariants) {
    EXPECT_NO_THROW((void)parse_regions(nlohmann::json::object()));
    EXPECT_NO_THROW((void)parse_regions(nlohmann::json::parse(R"json({"regions":"United States"})json")));
    EXPECT_NO_THROW((void)parse_regions(nlohmann::json::parse(R"json({"regions":"UnitedKingdom"})json")));
    EXPECT_NO_THROW((void)parse_regions(nlohmann::json::parse(R"json({"regions":"China"})json")));
    EXPECT_NO_THROW((void)parse_regions(nlohmann::json::parse(R"json({"regions":"Japan"})json")));
    EXPECT_NO_THROW((void)parse_regions(
        nlohmann::json::parse(R"json({"regions":["UnitedStates","United Kingdom","China","Japan"]})json")
    ));
    EXPECT_THROW((void)parse_regions(nlohmann::json::parse(R"json({"regions":"Mars"})json")), std::invalid_argument);
    EXPECT_THROW((void)parse_regions(nlohmann::json::parse(R"json({"regions":["Mars"]})json")), std::invalid_argument);
    EXPECT_THROW((void)parse_regions(nlohmann::json::parse(R"json({"regions":1})json")), std::invalid_argument);
}

TEST(ArrayParserTest, ParseGendersVariants) {
    EXPECT_NO_THROW((void)parse_genders(nlohmann::json::object()));
    EXPECT_NO_THROW((void)parse_genders(nlohmann::json::parse(R"json({"genders":"M"})json")));
    EXPECT_NO_THROW((void)parse_genders(nlohmann::json::parse(R"json({"genders":"Female"})json")));
    EXPECT_NO_THROW((void)parse_genders(nlohmann::json::parse(R"json({"genders":["Male","F"]})json")));
    EXPECT_THROW((void)parse_genders(nlohmann::json::parse(R"json({"genders":"X"})json")), std::invalid_argument);
    EXPECT_THROW((void)parse_genders(nlohmann::json::parse(R"json({"genders":["X"]})json")), std::invalid_argument);
    EXPECT_THROW((void)parse_genders(nlohmann::json::parse(R"json({"genders":1})json")), std::invalid_argument);
}

TEST(ArrayParserTest, ParseCardTypesVariants) {
    EXPECT_NO_THROW((void)parse_card_types(nlohmann::json::object()));
    EXPECT_NO_THROW((void)parse_card_types(nlohmann::json::parse(R"json({"card_type":"AmericanExpress"})json")));
    EXPECT_NO_THROW((void)parse_card_types(nlohmann::json::parse(R"json({"card_type":"JCB"})json")));
    EXPECT_NO_THROW((void)parse_card_types(nlohmann::json::parse(R"json({"card_type":"Master Card"})json")));
    EXPECT_NO_THROW((void)parse_card_types(nlohmann::json::parse(R"json({"card_type":"UnionPay"})json")));
    EXPECT_NO_THROW((void)parse_card_types(nlohmann::json::parse(R"json({"card_type":"Visa"})json")));
    EXPECT_NO_THROW((void)parse_card_types(
        nlohmann::json::parse(R"json({"card_types":["American Express","JCB","MasterCard","Union Pay","Visa"]})json")
    ));
    EXPECT_THROW((void)parse_card_types(nlohmann::json::parse(R"json({"card_type":"Unknown"})json")), std::invalid_argument);
    EXPECT_THROW((void)parse_card_types(nlohmann::json::parse(R"json({"card_types":["Unknown"]})json")), std::invalid_argument);
    EXPECT_THROW((void)parse_card_types(nlohmann::json::parse(R"json({"card_types":1})json")), std::invalid_argument);
}

TEST(ArrayParserTest, ParseBarcodeVariants) {
    EXPECT_NO_THROW((void)parse_barcode_type(nlohmann::json::object()));
    EXPECT_NO_THROW((void)parse_barcode_type(nlohmann::json::parse(R"json({"barcode_type":"EAN8"})json")));
    EXPECT_NO_THROW((void)parse_barcode_type(nlohmann::json::parse(R"json({"barcode_type":"EAN13"})json")));
    EXPECT_NO_THROW((void)parse_barcode_type(nlohmann::json::parse(R"json({"barcode_type":"UPCA"})json")));
    EXPECT_NO_THROW((void)parse_barcode_type(nlohmann::json::parse(R"json({"barcode_type":"UPCE"})json")));
    EXPECT_NO_THROW((void)parse_barcode_type(nlohmann::json::parse(R"json({"barcode_type":"ISBN"})json")));
    EXPECT_NO_THROW((void)parse_barcode_type(
        nlohmann::json::parse(R"json({"barcode_types":["EAN8","EAN13","UPCA","UPCE","ISBN"]})json")
    ));
    EXPECT_THROW((void)parse_barcode_type(nlohmann::json::parse(R"json({"barcode_type":"X"})json")), std::invalid_argument);
    EXPECT_THROW((void)parse_barcode_type(nlohmann::json::parse(R"json({"barcode_types":[]})json")), std::invalid_argument);
    EXPECT_THROW((void)parse_barcode_type(nlohmann::json::parse(R"json({"barcode_types":1})json")), std::invalid_argument);
    EXPECT_THROW((void)parse_barcode_type(nlohmann::json::parse(R"json({"barcode_types":["X"]})json")), std::invalid_argument);
}

TEST(ArrayParserTest, ParseOperatingSystemsVariants) {
    EXPECT_NO_THROW((void)parse_operating_systems(nlohmann::json::object()));
    EXPECT_NO_THROW((void)parse_operating_systems(nlohmann::json::parse(R"json({"operating_systems":"Windows"})json")));
    EXPECT_NO_THROW((void)parse_operating_systems(nlohmann::json::parse(R"json({"operating_systems":"macOS"})json")));
    EXPECT_NO_THROW((void)parse_operating_systems(nlohmann::json::parse(R"json({"operating_systems":"Linux"})json")));
    EXPECT_NO_THROW((void)parse_operating_systems(
        nlohmann::json::parse(R"json({"operating_systems":["Windows","macOS","Linux"]})json")
    ));
    EXPECT_THROW(
        (void)parse_operating_systems(nlohmann::json::parse(R"json({"operating_systems":"DOS"})json")),
        std::invalid_argument
    );
    EXPECT_THROW((void)parse_operating_systems(nlohmann::json::parse(R"json({"operating_systems":[]})json")), std::invalid_argument);
    EXPECT_THROW((void)parse_operating_systems(nlohmann::json::parse(R"json({"operating_systems":["DOS"]})json")), std::invalid_argument);
    EXPECT_THROW((void)parse_operating_systems(nlohmann::json::parse(R"json({"operating_systems":1})json")), std::invalid_argument);
}

TEST(ArrayParserTest, ParseCountryCodesAndUseTranslation) {
    EXPECT_NO_THROW((void)parse_country_codes_standard(nlohmann::json::object()));
    EXPECT_NO_THROW((void)parse_country_codes_standard(nlohmann::json::parse(R"json({"country_codes_standard":"None"})json")));
    EXPECT_NO_THROW((void)parse_country_codes_standard(
        nlohmann::json::parse(R"json({"country_codes_standard":"ISO_3166_1_alpha_2"})json")
    ));
    EXPECT_NO_THROW((void)parse_country_codes_standard(
        nlohmann::json::parse(R"json({"country_codes_standard":"ISO3166Alpha3"})json")
    ));
    EXPECT_THROW(
        (void)parse_country_codes_standard(nlohmann::json::parse(R"json({"country_codes_standard":"X"})json")),
        std::invalid_argument
    );

    EXPECT_FALSE(parse_use_translation(nlohmann::json::object()));
    EXPECT_TRUE(parse_use_translation(nlohmann::json::parse(R"json({"use_translation":true})json")));
}

}  // namespace
