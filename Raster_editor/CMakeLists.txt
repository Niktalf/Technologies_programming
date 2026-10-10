cmake_minimum_required(VERSION 3.28)
project(raster_editor C)

set(CMAKE_C_STANDARD 99)

add_library(
        raster_editor_core
        src/commands.c
        src/editor.c
        src/fill.c
        src/image.c
        src/input.c
        src/pixel.c
        src/ppm.c
)
target_include_directories(raster_editor_core PUBLIC include)

add_executable(${PROJECT_NAME} src/main.c)
target_link_libraries(${PROJECT_NAME} PRIVATE raster_editor_core)