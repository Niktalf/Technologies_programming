cmake_minimum_required(VERSION 3.28)
project(log_analyzer C)

set(CMAKE_C_STANDARD 99)

add_library(
        log_analyzer_core
        src/log_file.c
        src/shell.c
        src/time_stamp.c
        src/level.c
)
target_include_directories(log_analyzer_core PUBLIC include)

add_executable(${PROJECT_NAME} src/main.c)
target_link_libraries(${PROJECT_NAME} PRIVATE log_analyzer_core)