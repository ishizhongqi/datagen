cmake_minimum_required(VERSION 3.22)

project(data-generator CXX)

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

target_link_libraries(data-generator
        PRIVATE
        faker
        nlohmann_json
        cxxopts
)

target_compile_features(data-generator PRIVATE cxx_std_20)
