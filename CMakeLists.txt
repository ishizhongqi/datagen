cmake_minimum_required(VERSION 3.22)

project(data-generator CXX)

add_subdirectory(third_party/faker)

add_library(nlohmann_json INTERFACE)
target_include_directories(nlohmann_json INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/nlohmann/include
)

add_subdirectory(src)

target_link_libraries(data-generator
        PRIVATE
        faker
        nlohmann_json
)

target_compile_features(data-generator PRIVATE cxx_std_20)
