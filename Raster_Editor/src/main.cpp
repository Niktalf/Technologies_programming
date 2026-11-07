#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "editor.hpp"

// Практика 9: проект переписан на C++. Всё, что умел редактор на практике 8,
// осталось, но устроено классами: цвет, изображение и редактор.
//
//   ./raster                             — интерактивный режим
//   ./raster <file> <operation> [args]   — одна операция, результат в файл

namespace {

// Ошибки запуска, после которых программа завершается с ненулевым кодом,
// идут в поток ошибок. Ответы на команды — в обычный вывод.
void print_usage(const char* program) {
    std::cerr << "Usage:\n"
              << "  " << program << "                            interactive mode\n"
              << "  " << program
              << " <file> <operation> [args]  one operation, result to the file\n"
              << "Example:\n"
              << "  " << program << " checker.ppm gen checker 16\n";
}

// Редактор весит шесть мегабайт: два изображения с массивами наибольшего
// размера внутри. В стеке ему не место — стек обычно один-восемь мегабайт.
// static у локальной переменной: объект создаётся один раз и живёт
// до конца программы, но лежит не в стеке, а рядом с глобальными данными.
Editor& the_editor() {
    static Editor editor;
    return editor;
}

// Одна операция. Если после неё есть изображение, оно записывается в файл:
// так редактор можно вызывать из скриптов для генерации тестовых картинок.
int run_once(int argc, char* argv[]) {
    std::cout << "Arguments: " << argc << '\n';
    for (int i = 0; i < argc; ++i) {
        std::cout << "  argv[" << i << "] = " << argv[i] << '\n';
    }
    if (argc < 3) {
        print_usage(argv[0]);
        return 1;
    }
    Editor& editor = the_editor();
    editor.set_path(argv[1]);
    std::vector<std::string> words(argv + 2, argv + argc);
    editor.execute(words);
    if (!editor.loaded()) {
        return 0;
    }
    ImageStatus status = editor.save_to_path();
    if (status != ImageStatus::ok) {
        std::cerr << "Cannot write the file " << editor.path() << ": "
                  << Image::status_text(status) << '\n';
        return 1;
    }
    std::cout << "Written: " << editor.path() << '\n';
    return 0;
}

// Интерактивный режим: операции по одной, пока не будет команды выхода
// или ввод не кончится. Обе причины сводятся к одному флагу running.
int run_interactive() {
    Editor& editor = the_editor();
    std::cout << "Raster editor. Type help for the list of operations.\n";
    bool running = true;
    while (running) {
        std::cout << "> " << std::flush;
        std::string line;
        if (!std::getline(std::cin, line)) {
            running = false;
        } else {
            // Строка любой длины: std::string растёт сам, переполнять нечего.
            std::istringstream stream(line);
            std::vector<std::string> words;
            std::string word;
            while (stream >> word) {
                words.push_back(word);
            }
            running = editor.execute(words);
        }
    }
    std::cout << "Done.\n";
    return 0;
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc > 1) {
        std::cout << "Mode: one operation from the command line\n";
        return run_once(argc, argv);
    }
    std::cout << "Mode: interactive\n";
    return run_interactive();
}
