cmake_minimum_required(VERSION 3.16)
project(raster_editor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(${PROJECT_NAME}_core
    src/color.cpp
    src/editor.cpp
    src/filter.cpp
    src/image_stats.cpp
    src/image.cpp
    src/pixel_ops.cpp
    src/ppm_file.cpp
    src/status.cpp)
target_include_directories(${PROJECT_NAME}_core PUBLIC include)

add_executable(${PROJECT_NAME} src/main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_NAME}_core)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(${PROJECT_NAME}_core PRIVATE -Wall -Wextra -pedantic)
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic)
endif()
