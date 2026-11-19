cmake_minimum_required(VERSION 3.16)
project(dungeon LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(${PROJECT_NAME}_core
    src/creature.cpp
    src/exit_search.cpp
    src/game.cpp
    src/hero.cpp
    src/level_generator.cpp
    src/map_view.cpp
    src/map.cpp
    src/monster_list.cpp
    src/monster_query.cpp
    src/monster.cpp
    src/move_rule.cpp)
target_include_directories(${PROJECT_NAME}_core PUBLIC include)

add_executable(${PROJECT_NAME} src/main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_NAME}_core)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(${PROJECT_NAME}_core PRIVATE -Wall -Wextra -pedantic)
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic)
endif()
