cmake_minimum_required(VERSION 3.22)

set(MAIN_SOURCE main.cpp)

set(CLI_DIRECTORY cli)

set(GENERATOR_DIRECTORY generators)

set(GENERATOR_SOURCE
        ${GENERATOR_DIRECTORY}/generator_base.h
        ${GENERATOR_DIRECTORY}/generator_registry.cpp
        ${GENERATOR_DIRECTORY}/generator_registry.h
        ${GENERATOR_DIRECTORY}/computer_generators.cpp
        ${GENERATOR_DIRECTORY}/computer_generators.h
        ${GENERATOR_DIRECTORY}/business_generators.cpp
        ${GENERATOR_DIRECTORY}/business_generators.h
        ${GENERATOR_DIRECTORY}/datetime_generators.cpp
        ${GENERATOR_DIRECTORY}/datetime_generators.h
        ${GENERATOR_DIRECTORY}/location_generators.cpp
        ${GENERATOR_DIRECTORY}/location_generators.h
        ${GENERATOR_DIRECTORY}/number_generators.cpp
        ${GENERATOR_DIRECTORY}/number_generators.h
        ${GENERATOR_DIRECTORY}/payment_generators.cpp
        ${GENERATOR_DIRECTORY}/payment_generators.h
        ${GENERATOR_DIRECTORY}/person_generators.cpp
        ${GENERATOR_DIRECTORY}/person_generators.h
        ${GENERATOR_DIRECTORY}/product_generators.cpp
        ${GENERATOR_DIRECTORY}/product_generators.h
        ${GENERATOR_DIRECTORY}/string_generators.cpp
        ${GENERATOR_DIRECTORY}/string_generators.h
        ${GENERATOR_DIRECTORY}/utility_generators.cpp
        ${GENERATOR_DIRECTORY}/utility_generators.h
        ${GENERATOR_DIRECTORY}/linkage_helper.cpp
        ${GENERATOR_DIRECTORY}/linkage_helper.h
        ${GENERATOR_DIRECTORY}/array_parser.cpp
        ${GENERATOR_DIRECTORY}/array_parser.h
        ${GENERATOR_DIRECTORY}/override_rules.cpp
        ${GENERATOR_DIRECTORY}/override_rules.h
)

add_executable(data-generator ${MAIN_SOURCE} ${GENERATOR_SOURCE})

target_include_directories(data-generator
        PRIVATE
        generator
)
