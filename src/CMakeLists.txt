cmake_minimum_required(VERSION 3.22)

set(MAIN_SOURCE main.cpp)

set(CLI_DIRECTORY cli)

set(GENERATOR_DIRECTORY generators)

set(CLI_SOURCE
        ${CLI_DIRECTORY}/cli_dispatcher.cpp
        ${CLI_DIRECTORY}/cli_dispatcher.h
        ${CLI_DIRECTORY}/cli_shared.cpp
        ${CLI_DIRECTORY}/cli_shared.h
        ${CLI_DIRECTORY}/generator_catalog.cpp
        ${CLI_DIRECTORY}/generator_catalog.h
        ${CLI_DIRECTORY}/command_init.cpp
        ${CLI_DIRECTORY}/command_init.h
        ${CLI_DIRECTORY}/command_preview.cpp
        ${CLI_DIRECTORY}/command_preview.h
        ${CLI_DIRECTORY}/command_generate.cpp
        ${CLI_DIRECTORY}/command_generate.h
        ${CLI_DIRECTORY}/command_describe.cpp
        ${CLI_DIRECTORY}/command_describe.h
        ${CLI_DIRECTORY}/command_list.cpp
        ${CLI_DIRECTORY}/command_list.h
        ${CLI_DIRECTORY}/command_validate.cpp
        ${CLI_DIRECTORY}/command_validate.h
        ${CLI_DIRECTORY}/command_help.cpp
        ${CLI_DIRECTORY}/command_help.h
)

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

add_executable(data-generator ${MAIN_SOURCE} ${CLI_SOURCE} ${GENERATOR_SOURCE})

target_include_directories(data-generator
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
