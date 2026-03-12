cmake_minimum_required(VERSION 3.22)

set(MAIN_SOURCE main.cpp)

set(APP_DIRECTORY app)

set(CLI_DIRECTORY cli)
set(CONFIG_DIRECTORY config)
set(ENGINE_DIRECTORY engine)
set(UTILS_DIRECTORY utils)

set(GENERATOR_CORE_DIRECTORY generators/core)
set(GENERATOR_TYPES_DIRECTORY generators/types)

set(OUTPUT_DIRECTORY output)
set(OUTPUT_FILE_DIRECTORY output/file)
set(OUTPUT_DATABASE_DIRECTORY output/database)
set(OUTPUT_DATABASE_DRIVERS_DIRECTORY output/database/drivers)
set(LOGGING_DIRECTORY logging)

set(APP_SOURCE
        ${APP_DIRECTORY}/run.cpp
        ${APP_DIRECTORY}/run.h
)

set(CONFIG_SOURCE
        ${CONFIG_DIRECTORY}/configuration.cpp
        ${CONFIG_DIRECTORY}/configuration.h
        ${CONFIG_DIRECTORY}/generator_catalog.cpp
        ${CONFIG_DIRECTORY}/generator_catalog.h
)

set(ENGINE_SOURCE
        ${ENGINE_DIRECTORY}/executor.cpp
        ${ENGINE_DIRECTORY}/executor.h
)

set(UTILS_SOURCE
        ${UTILS_DIRECTORY}/env_utils.cpp
        ${UTILS_DIRECTORY}/env_utils.h
        ${UTILS_DIRECTORY}/workspace.cpp
        ${UTILS_DIRECTORY}/workspace.h
)

set(CLI_SOURCE
        ${CLI_DIRECTORY}/cli_dispatcher.cpp
        ${CLI_DIRECTORY}/cli_dispatcher.h
        ${CLI_DIRECTORY}/cli_shared.cpp
        ${CLI_DIRECTORY}/cli_shared.h
        ${CLI_DIRECTORY}/command_init.cpp
        ${CLI_DIRECTORY}/command_init.h
        ${CLI_DIRECTORY}/command_preview.cpp
        ${CLI_DIRECTORY}/command_preview.h
        ${CLI_DIRECTORY}/command_run.cpp
        ${CLI_DIRECTORY}/command_run.h
        ${CLI_DIRECTORY}/command_info.cpp
        ${CLI_DIRECTORY}/command_info.h
        ${CLI_DIRECTORY}/command_drivers.cpp
        ${CLI_DIRECTORY}/command_drivers.h
        ${CLI_DIRECTORY}/command_check.cpp
        ${CLI_DIRECTORY}/command_check.h
        ${CLI_DIRECTORY}/command_schema.cpp
        ${CLI_DIRECTORY}/command_schema.h
        ${CLI_DIRECTORY}/command_help.cpp
        ${CLI_DIRECTORY}/command_help.h
)

set(GENERATOR_SOURCE
        ${GENERATOR_CORE_DIRECTORY}/generator_base.h
        ${GENERATOR_CORE_DIRECTORY}/generator_registry.cpp
        ${GENERATOR_CORE_DIRECTORY}/generator_registry.h
        ${GENERATOR_CORE_DIRECTORY}/linkage_helper.cpp
        ${GENERATOR_CORE_DIRECTORY}/linkage_helper.h
        ${GENERATOR_CORE_DIRECTORY}/array_parser.cpp
        ${GENERATOR_CORE_DIRECTORY}/array_parser.h
        ${GENERATOR_CORE_DIRECTORY}/override_rules.cpp
        ${GENERATOR_CORE_DIRECTORY}/override_rules.h
        ${GENERATOR_TYPES_DIRECTORY}/computer_generators.cpp
        ${GENERATOR_TYPES_DIRECTORY}/computer_generators.h
        ${GENERATOR_TYPES_DIRECTORY}/business_generators.cpp
        ${GENERATOR_TYPES_DIRECTORY}/business_generators.h
        ${GENERATOR_TYPES_DIRECTORY}/datetime_generators.cpp
        ${GENERATOR_TYPES_DIRECTORY}/datetime_generators.h
        ${GENERATOR_TYPES_DIRECTORY}/location_generators.cpp
        ${GENERATOR_TYPES_DIRECTORY}/location_generators.h
        ${GENERATOR_TYPES_DIRECTORY}/number_generators.cpp
        ${GENERATOR_TYPES_DIRECTORY}/number_generators.h
        ${GENERATOR_TYPES_DIRECTORY}/payment_generators.cpp
        ${GENERATOR_TYPES_DIRECTORY}/payment_generators.h
        ${GENERATOR_TYPES_DIRECTORY}/person_generators.cpp
        ${GENERATOR_TYPES_DIRECTORY}/person_generators.h
        ${GENERATOR_TYPES_DIRECTORY}/product_generators.cpp
        ${GENERATOR_TYPES_DIRECTORY}/product_generators.h
        ${GENERATOR_TYPES_DIRECTORY}/string_generators.cpp
        ${GENERATOR_TYPES_DIRECTORY}/string_generators.h
        ${GENERATOR_TYPES_DIRECTORY}/utility_generators.cpp
        ${GENERATOR_TYPES_DIRECTORY}/utility_generators.h
)

set(OUTPUT_SOURCE
        ${OUTPUT_DIRECTORY}/output_backend.h
        ${OUTPUT_DIRECTORY}/output_backend.cpp
        ${OUTPUT_DIRECTORY}/file_backend.h
        ${OUTPUT_DIRECTORY}/file_backend.cpp
        ${OUTPUT_DIRECTORY}/database_backend.h
        ${OUTPUT_DIRECTORY}/database_backend.cpp
        ${OUTPUT_FILE_DIRECTORY}/csv_writer.h
        ${OUTPUT_FILE_DIRECTORY}/csv_writer.cpp
        ${OUTPUT_FILE_DIRECTORY}/json_writer.h
        ${OUTPUT_FILE_DIRECTORY}/json_writer.cpp
        ${OUTPUT_FILE_DIRECTORY}/sql_writer.h
        ${OUTPUT_FILE_DIRECTORY}/sql_writer.cpp
)

set(DATABASE_SOURCE
        ${OUTPUT_DATABASE_DIRECTORY}/db_metadata.h
        ${OUTPUT_DATABASE_DIRECTORY}/db_metadata.cpp
        ${OUTPUT_DATABASE_DIRECTORY}/db_url_parser.h
        ${OUTPUT_DATABASE_DIRECTORY}/db_url_parser.cpp
        ${OUTPUT_DATABASE_DIRECTORY}/db_schema_validator.h
        ${OUTPUT_DATABASE_DIRECTORY}/db_schema_validator.cpp
        ${OUTPUT_DATABASE_DIRECTORY}/type_adapter.h
        ${OUTPUT_DATABASE_DIRECTORY}/type_adapter.cpp
        ${OUTPUT_DATABASE_DIRECTORY}/odbc_registry.h
        ${OUTPUT_DATABASE_DIRECTORY}/odbc_registry.cpp
        ${OUTPUT_DATABASE_DRIVERS_DIRECTORY}/idatabase_driver.h
        ${OUTPUT_DATABASE_DRIVERS_DIRECTORY}/idatabase_driver.cpp
        ${OUTPUT_DATABASE_DRIVERS_DIRECTORY}/odbc_driver.h
        ${OUTPUT_DATABASE_DRIVERS_DIRECTORY}/odbc_driver.cpp
        ${OUTPUT_DATABASE_DRIVERS_DIRECTORY}/sqlite_driver.h
        ${OUTPUT_DATABASE_DRIVERS_DIRECTORY}/sqlite_driver.cpp
)

set(LOGGING_SOURCE
        ${LOGGING_DIRECTORY}/logger.h
        ${LOGGING_DIRECTORY}/logger.cpp
)

add_library(data-generator-lib
        ${APP_SOURCE}
        ${CLI_SOURCE}
        ${GENERATOR_SOURCE}
        ${CONFIG_SOURCE}
        ${ENGINE_SOURCE}
        ${UTILS_SOURCE}
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
