cmake_minimum_required(VERSION 3.16)
project(log_analyzer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(${PROJECT_NAME}_core
    src/binary_log.cpp
    src/file_reader.cpp
    src/level.cpp
    src/log_book.cpp
    src/log_generator.cpp
    src/log_insights.cpp
    src/log_parser.cpp
    src/log_record.cpp
    src/log_report.cpp
    src/log_source.cpp
    src/log_view.cpp
    src/record_filter.cpp
    src/shell.cpp
    src/status.cpp
    src/timestamp.cpp)
target_include_directories(${PROJECT_NAME}_core PUBLIC include)

add_executable(${PROJECT_NAME} src/main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_NAME}_core)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(${PROJECT_NAME}_core PRIVATE -Wall -Wextra -pedantic)
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic)
endif()
