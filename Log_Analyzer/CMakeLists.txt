cmake_minimum_required(VERSION 4.0)
project(log_analyzer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(
        ${PROJECT_NAME}_core
        src/level.cpp
        src/log_book.cpp
        src/log_generator.cpp
        src/log_record.cpp
        src/log_stats.cpp
        src/log_view.cpp
        src/shell.cpp
        src/status.cpp
        src/timestamp.cpp
)

target_include_directories(${PROJECT_NAME}_core PUBLIC include)

add_executable(
        ${PROJECT_NAME}
        src/main.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_NAME}_core)
