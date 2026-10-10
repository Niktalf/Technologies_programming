cmake_minimum_required(VERSION 3.28)
project(kvs C)

set(CMAKE_C_STANDARD 99)

add_library(
        kvs_core
        src/cli.c
        src/range.c
        src/record.c
        src/session.c
        src/store.c
)
target_include_directories(kvs_core PUBLIC include)

add_executable(${PROJECT_NAME} src/main.c)
target_link_libraries(${PROJECT_NAME} PRIVATE kvs_core)