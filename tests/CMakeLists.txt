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
    test_executor.cpp
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
