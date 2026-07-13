cmake_minimum_required(VERSION 3.22)

project(datagen VERSION 1.0.0 LANGUAGES CXX)

option(DATAGEN_ENABLE_COVERAGE "Enable coverage instrumentation" OFF)

set(FAKER_BUILD_SHARED OFF CACHE BOOL "faker shared build" FORCE)
set(FAKER_BUILD_EXAMPLE OFF CACHE BOOL "faker example build" FORCE)
set(FAKER_BUILD_TESTS OFF CACHE BOOL "faker test build" FORCE)
set(FAKER_CODE_COVERAGE OFF CACHE BOOL "faker coverage build" FORCE)

# faker version 1.0.0 https://github.com/ishizhongqi/faker
add_subdirectory(third_party/faker)

# nlohmann/json version 3.12.0 https://github.com/nlohmann/json
add_library(nlohmann_json INTERFACE)
target_include_directories(nlohmann_json INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/nlohmann/include
)

# cxxopts
add_library(cxxopts INTERFACE)
target_include_directories(cxxopts INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/cxxopts
)

add_subdirectory(src)

find_package(ODBC REQUIRED)
find_package(SQLite3 REQUIRED)

target_link_libraries(datagen-lib
        PUBLIC
        faker
        nlohmann_json
        cxxopts
        ODBC::ODBC
        SQLite::SQLite3
)

target_compile_definitions(datagen-lib
        PUBLIC
        DATAGEN_VERSION="${PROJECT_VERSION}"
)

target_compile_features(datagen-lib PUBLIC cxx_std_20)

set(DATAGEN_SCHEMA_DIR "${CMAKE_SOURCE_DIR}/schema")
set(DATAGEN_SCHEMA_FILE "${DATAGEN_SCHEMA_DIR}/datagen.schema.json")

add_custom_command(
    OUTPUT "${DATAGEN_SCHEMA_FILE}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${DATAGEN_SCHEMA_DIR}"
    COMMAND $<TARGET_FILE:datagen> schema "${DATAGEN_SCHEMA_FILE}"
    DEPENDS datagen
    COMMENT "Generating JSON Schema: ${DATAGEN_SCHEMA_FILE}"
    VERBATIM
)

add_custom_target(generate-schema ALL DEPENDS "${DATAGEN_SCHEMA_FILE}")

if(DATAGEN_ENABLE_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(datagen-lib
            PUBLIC
            -O0
            -g
            -fprofile-instr-generate
            -fcoverage-mapping
    )
    target_link_options(datagen-lib
            PUBLIC
            -fprofile-instr-generate
            -fcoverage-mapping
    )
endif()

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
