cmake_minimum_required(VERSION 3.22)

project(data_generator CXX)

include(FetchContent)

FetchContent_Declare(
        faker
        GIT_REPOSITORY https://github.com/ishizhongqi/faker.git
        GIT_TAG develop
)

FetchContent_MakeAvailable(faker)

add_executable(data_generator main.cpp)

set(TEST_COMPILE_FEATURES cxx_std_20)

target_link_libraries(data_generator
    PRIVATE
        faker
)

target_compile_features(data_generator PRIVATE ${TEST_COMPILE_FEATURES})

target_include_directories(data_generator
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
)
