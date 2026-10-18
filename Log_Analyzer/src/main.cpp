#include <iostream>

#include "shell.hpp"

// Практика 9: проект переписан на C++. Всё, что умел анализатор на практике 8,
// осталось, но устроено классами: запись, журнал, фильтр, порядок просмотра,
// статистика, условие выборки и оболочка, которая всё это объединяет.
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
        LogStatus status = shell.open(argv[1]);
        if (status != LogStatus::ok) {
            std::cerr << "Error: " << status_text(status) << " (" << argv[1] << ")\n";
            return 1;
        }
    }
    shell.run();
    return 0;
}
