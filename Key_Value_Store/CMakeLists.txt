cmake_minimum_required(VERSION 4.0)
project(key_value_sore LANGUAGES CXX)

set(CMAKE_Cxx_STANDARD 17)
set(CMAKE_Cxx_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(
        ${PROJECT_NAME}_core
        src/console.cpp
        src/journal_file.cpp
        src/parser.cpp
        src/record.cpp
        src/session.cpp
        src/sorted_pairs.cpp
        src/store.cpp
)
target_include_directories(${PROJECT_NAME}_core PUBLIC include)

add_executable(
        ${PROJECT_NAME}
        src/main.cpp
)
target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_NAME}_core)

add_executable(
        ${PROJECT_NAME}_demo
        demo/phonebook.cpp
)
target_link_libraries(${PROJECT_NAME}_demo PRIVATE ${PROJECT_NAME}_core)
