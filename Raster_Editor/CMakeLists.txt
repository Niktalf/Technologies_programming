cmake_minimum_required(VERSION 4.0)
project(raster_editor LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_library(
        ${PROJECT_NAME}_core
        src/commands.c
        src/editor.c
        src/fill.c
        src/image.c
        src/input.c
        src/pixel.c
        src/ppm.c
)

target_include_directories(${PROJECT_NAME}_core PUBLIC include)

add_executable(
        ${PROJECT_NAME}
        src/main.c
)

target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_NAME}_core)
