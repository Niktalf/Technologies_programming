cmake_minimum_required(VERSION 4.0)
project(key_value_sore LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_library(
        ${PROJECT_NAME}_core
        src/cli.c
        src/range.c
        src/record.c
        src/session.c
        src/store.c
)

target_include_directories(${PROJECT_NAME}_core PUBLIC include)

add_executable(
        ${PROJECT_NAME}
        src/main.c
)

target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_NAME}_core)
