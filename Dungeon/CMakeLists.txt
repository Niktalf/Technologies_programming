cmake_minimum_required(VERSION 4.0)
project(dungeon LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_library(
        ${PROJECT_NAME}_core
        src/command.c
        src/entities.c
        src/game.c
        src/level.c
        src/map.c
        src/reach.c
        src/status.c
)

target_include_directories(${PROJECT_NAME}_core PUBLIC include)

add_executable(
        ${PROJECT_NAME}
        src/main.c
)

target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_NAME}_core)
