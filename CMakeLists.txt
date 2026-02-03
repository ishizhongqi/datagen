cmake_minimum_required(VERSION 3.22)

project(data_generator CXX)

add_subdirectory(third_party/faker)

add_executable(data_generator src/main.cpp
        src/generator/generator_base.h
        src/generator/generator_registry.cpp
        src/generator/generator_registry.h
        src/generator/computer_generators.cpp
        src/generator/computer_generators.h
        src/generator/array_parser.cpp
        src/generator/array_parser.h)

set(TEST_COMPILE_FEATURES cxx_std_20)

target_link_libraries(data_generator
        PRIVATE
        faker
)

target_compile_features(data_generator PRIVATE ${TEST_COMPILE_FEATURES})

target_include_directories(data_generator
        PRIVATE
        src/third_party
        src/generator
)
