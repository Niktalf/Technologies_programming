cmake_minimum_required(VERSION 3.28)
project(log_analyzer C)

set(CMAKE_C_STANDARD 99)

add_library(
        log_analyzer_core
        src/generator.c
        src/level.c
        src/logfile.c
        src/parse.c
        src/shell.c
        src/sort.c
        src/stats.c
        src/text.c
        src/timestamp.c
        src/view.c
)
target_include_directories(log_analyzer_core PUBLIC include)

add_executable(${PROJECT_NAME} src/main.c)
target_link_libraries(${PROJECT_NAME} PRIVATE log_analyzer_core)