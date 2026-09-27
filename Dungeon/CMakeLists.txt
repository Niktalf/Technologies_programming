cmake_minimum_required(VERSION 3.28)
project(dungeon C)

set(CMAKE_C_STANDARD 99)

add_library(
        dungeon_core
        src/command.c
        src/game.c
        src/map.c
        src/hero.c
)
target_include_directories(dungeon_core PUBLIC include)

add_executable(${PROJECT_NAME} src/main.c)
target_link_libraries(${PROJECT_NAME} PRIVATE dungeon_core)