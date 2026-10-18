#include <iostream>

#include "shell.hpp"

// Практика 10: файлом владеет FileReader, журнал загружается в конструкторе.
// Всё, что умел анализатор на практике 9, осталось; добавились команды demo
// и stress — опыты со временем жизни и исключением.
//
//   ./logan              — интерактивный режим, журнал загружается командой load
//   ./logan sample.log   — сразу загрузить журнал и напечатать сводку
int main(int argc, char* argv[]) {
    if (argc > 2) {
        std::cerr << "Usage: " << argv[0] << " [log file]\n";
        return 1;
    }
    Shell shell;
    // Файл можно указать сразу, а можно загрузить потом командой load.
    // Ошибка запуска — в поток ошибок и код 1, ответы на команды — в обычный вывод.
    if (argc == 2) {
        try {
            shell.open(argv[1]);
        } catch (const LogError& error) {
            std::cerr << "Error: " << error.what() << '\n';
            return 1;
        }
    }
    shell.run();
    return 0;
}
