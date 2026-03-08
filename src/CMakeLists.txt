cmake_minimum_required(VERSION 3.22)

set(MAIN_SOURCE main.cpp)

set(APP_DIRECTORY app)

set(CORE_DIRECTORY core)

set(CLI_DIRECTORY cli)

set(GENERATOR_DIRECTORY generators)
set(OUTPUT_DIRECTORY output)
set(DATABASE_DIRECTORY database)
set(DATABASE_DRIVER_DIRECTORY database/driver)
set(LOGGING_DIRECTORY logging)

set(APP_SOURCE
        ${APP_DIRECTORY}/run.cpp
        ${APP_DIRECTORY}/run.h
)

set(CORE_SOURCE
        ${CORE_DIRECTORY}/configuration.cpp
        ${CORE_DIRECTORY}/configuration.h
        ${CORE_DIRECTORY}/executor.cpp
        ${CORE_DIRECTORY}/executor.h
        ${CORE_DIRECTORY}/serialization.cpp
        ${CORE_DIRECTORY}/serialization.h
        ${CORE_DIRECTORY}/workspace.cpp
        ${CORE_DIRECTORY}/workspace.h
)

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
        ${CLI_DIRECTORY}/command_drivers.cpp
        ${CLI_DIRECTORY}/command_drivers.h
        ${CLI_DIRECTORY}/command_validate.cpp
        ${CLI_DIRECTORY}/command_validate.h
        ${CLI_DIRECTORY}/command_schema.cpp
        ${CLI_DIRECTORY}/command_schema.h
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

set(OUTPUT_SOURCE
        ${OUTPUT_DIRECTORY}/output_backend.h
        ${OUTPUT_DIRECTORY}/output_backend.cpp
        ${OUTPUT_DIRECTORY}/file_backend.h
        ${OUTPUT_DIRECTORY}/file_backend.cpp
        ${OUTPUT_DIRECTORY}/database_backend.h
        ${OUTPUT_DIRECTORY}/database_backend.cpp
)

set(DATABASE_SOURCE
        ${DATABASE_DIRECTORY}/db_metadata.h
        ${DATABASE_DIRECTORY}/db_metadata.cpp
        ${DATABASE_DIRECTORY}/db_url_parser.h
        ${DATABASE_DIRECTORY}/db_url_parser.cpp
        ${DATABASE_DIRECTORY}/db_schema_validator.h
        ${DATABASE_DIRECTORY}/db_schema_validator.cpp
        ${DATABASE_DIRECTORY}/type_adapter.h
        ${DATABASE_DIRECTORY}/type_adapter.cpp
        ${DATABASE_DIRECTORY}/odbc_registry.h
        ${DATABASE_DIRECTORY}/odbc_registry.cpp
        ${DATABASE_DRIVER_DIRECTORY}/idatabase_driver.h
        ${DATABASE_DRIVER_DIRECTORY}/idatabase_driver.cpp
        ${DATABASE_DRIVER_DIRECTORY}/odbc_driver.h
        ${DATABASE_DRIVER_DIRECTORY}/odbc_driver.cpp
)

set(LOGGING_SOURCE
        ${LOGGING_DIRECTORY}/logger.h
        ${LOGGING_DIRECTORY}/logger.cpp
)

add_library(data-generator-lib
        ${APP_SOURCE}
        ${CLI_SOURCE}
        ${GENERATOR_SOURCE}
        ${CORE_SOURCE}
        ${OUTPUT_SOURCE}
        ${DATABASE_SOURCE}
        ${LOGGING_SOURCE}
)

add_executable(data-generator ${MAIN_SOURCE})

target_link_libraries(data-generator PRIVATE data-generator-lib)

target_include_directories(data-generator-lib
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)
