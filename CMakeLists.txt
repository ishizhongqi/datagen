cmake_minimum_required(VERSION 3.22)

project(data_generator CXX)

add_subdirectory(third_party/faker)

add_executable(data_generator src/main.cpp
        src/generator/generator_base.h
        src/generator/generator_registry.cpp
        src/generator/generator_registry.h
        src/generator/computer_generators.cpp
        src/generator/computer_generators.h
        src/generator/business_generators.cpp
        src/generator/business_generators.h
        src/generator/datetime_generators.cpp
        src/generator/datetime_generators.h
        src/generator/location_generators.cpp
        src/generator/location_generators.h
        src/generator/number_generators.cpp
        src/generator/number_generators.h
        src/generator/payment_generators.cpp
        src/generator/payment_generators.h
        src/generator/person_generators.cpp
        src/generator/person_generators.h
        src/generator/product_generators.cpp
        src/generator/product_generators.h
        src/generator/string_generators.cpp
        src/generator/string_generators.h
        src/generator/utility_generators.cpp
        src/generator/utility_generators.h
        src/generator/array_parser.cpp
        src/generator/array_parser.h
        src/generator/override_rules.cpp
        src/generator/override_rules.h)

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
