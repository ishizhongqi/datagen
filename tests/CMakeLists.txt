cmake_minimum_required(VERSION 3.22)

include(CTest)

include(FetchContent)

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(data-generator-tests
    test_configuration.cpp
    test_configuration_edge.cpp
    test_executor.cpp
    test_array_parser.cpp
    test_override_rules.cpp
    test_db_metadata.cpp
    test_db_url_parser.cpp
    test_db_schema_validator.cpp
    test_type_adapter.cpp
    test_generators_smoke.cpp
    test_odbc_driver.cpp
    test_odbc_registry.cpp
    test_output_backend.cpp
    test_linkage_and_registry.cpp
    test_linkage_conflicts.cpp
    test_serialization.cpp
    test_utility_generators.cpp
    test_executor_edge.cpp
    test_cli_shared.cpp
    test_cli_commands.cpp
    test_file_backend.cpp
    test_file_writers.cpp
    test_database_backend.cpp
    test_sqlite_driver.cpp
    test_workspace.cpp
    test_run.cpp
)

target_link_libraries(data-generator-tests
    PRIVATE
    data-generator-lib
    GTest::gtest_main
)

target_compile_features(data-generator-tests PRIVATE cxx_std_20)

target_compile_definitions(data-generator-tests
    PRIVATE
    DATA_GENERATOR_TEST_INPUT_DIR="${CMAKE_CURRENT_SOURCE_DIR}/inputs"
)

include(GoogleTest)
gtest_discover_tests(data-generator-tests)
