cmake_minimum_required(VERSION 4.0)
project(log_analyzer LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_library(
        ${PROJECT_NAME}_core
        src/generator.c
        src/level.c
        src/logfile.c
        src/parse.c
        src/shell.c
        src/stats.c
        src/timestamp.c
)

target_include_directories(${PROJECT_NAME}_core PUBLIC include)

add_executable(
        ${PROJECT_NAME}
        src/main.c
)

target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_NAME}_core)
