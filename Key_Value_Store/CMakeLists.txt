cmake_minimum_required(VERSION 3.16)
project(key_value_sore LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Библиотека: запись, файл журнала, два движка и публичный класс KvStore.
# Каталог include/ — публичный: его получает каждый, кто линкуется с key_value_sore_core.
# Каталог src/ — закрытый: только для самой библиотеки.
add_library(key_value_sore_core STATIC
    src/engine.cpp
    src/journal_engine.cpp
    src/journal_file.cpp
    src/kvstore.cpp
    src/memory_engine.cpp
    src/record.cpp)
target_include_directories(key_value_sore_core PUBLIC include PRIVATE src)

# Консольная программа — инструмент самой библиотеки: ей нужны подробности
# журнала, поэтому ей открыт и каталог src/.
add_executable(key_value_sore
    src/conformance.cpp
    src/console.cpp
    src/main.cpp
    src/parser.cpp
    src/session.cpp
    src/sorted_pairs.cpp)
target_include_directories(key_value_sore PRIVATE src)
target_link_libraries(key_value_sore PRIVATE key_value_sore_core)

# Демонстрационные программы — клиенты: им виден только include/kvstore.hpp.
add_executable(key_value_sore_phonebook demo/phonebook.cpp)
target_link_libraries(key_value_sore_phonebook PRIVATE key_value_sore_core)

add_executable(key_value_sore_visits demo/visits.cpp)
target_link_libraries(key_value_sore_visits PRIVATE key_value_sore_core)

# Проверки движков: общий набор на обоих движках. Запуск — ctest.
enable_testing()
add_executable(engine_tests tests/engine_tests.cpp src/conformance.cpp)
target_include_directories(engine_tests PRIVATE src)
target_link_libraries(engine_tests PRIVATE key_value_sore_core)
add_test(NAME engines COMMAND engine_tests)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(key_value_sore_core PRIVATE -Wall -Wextra -pedantic)
    target_compile_options(key_value_sore PRIVATE -Wall -Wextra -pedantic)
    target_compile_options(key_value_sore_phonebook PRIVATE -Wall -Wextra -pedantic)
    target_compile_options(key_value_sore_visits PRIVATE -Wall -Wextra -pedantic)
    target_compile_options(engine_tests PRIVATE -Wall -Wextra -pedantic)
endif()
