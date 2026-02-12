cmake_minimum_required(VERSION 3.22)

project(data-generator CXX)

option(DATA_GENERATOR_ENABLE_COVERAGE "Enable coverage instrumentation" OFF)

set(FAKER_BUILD_SHARED OFF CACHE BOOL "faker shared build" FORCE)
set(FAKER_BUILD_EXAMPLE OFF CACHE BOOL "faker example build" FORCE)
set(FAKER_BUILD_TESTS OFF CACHE BOOL "faker test build" FORCE)
set(FAKER_CODE_COVERAGE OFF CACHE BOOL "faker coverage build" FORCE)

# faker
add_subdirectory(third_party/faker)

# nlohmann/json
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

target_link_libraries(data-generator-lib
        PUBLIC
        faker
        nlohmann_json
        cxxopts
)

target_compile_features(data-generator-lib PUBLIC cxx_std_20)

if(DATA_GENERATOR_ENABLE_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(data-generator-lib
            PUBLIC
            -O0
            -g
            -fprofile-instr-generate
            -fcoverage-mapping
    )
    target_link_options(data-generator-lib
            PUBLIC
            -fprofile-instr-generate
            -fcoverage-mapping
    )
endif()

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
