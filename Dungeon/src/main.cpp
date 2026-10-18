#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "game.hpp"

// Практика 9: проект переписан на C++. Всё, что умела игра на практике 8,
// осталось, но устроено классами: карта, герой, монстр, список монстров,
// генератор уровней, поиск выхода и игра, которая всё это объединяет.
//
//   ./dungeon                    — спросит размер и построит случайный уровень
//   ./dungeon levels/cave.txt    — уровень из файла
//
// main только читает строки и передаёт их игре. Ни карта, ни герой ввода
// не читают: они не знают, откуда пришла команда, и их можно использовать,
// например, в тестах без всякого ввода.

namespace {

// Разбирает ответ «ширина высота [зерно]». Возвращает true, если в нём
// два или три числа и ничего лишнего.
bool parse_size(const std::string& line, int& width, int& height, unsigned int& seed) {
    std::istringstream words(line);
    if (!(words >> width >> height)) {
        return false;
    }
    std::string rest;
    if (!(words >> rest)) {
        return true;   // зерна нет: остаётся 1
    }
    std::istringstream seed_word(rest);
    std::string extra;
    return static_cast<bool>(seed_word >> seed) && seed_word.eof() && !(words >> extra);
}

// Спрашивает размер случайного уровня, пока ответ негоден. Возвращает true,
// если уровень построен, и false, если ввод кончился.
bool ask_random_level(Game& game) {
    bool created = false;
    bool input_ended = false;
    while (!created && !input_ended) {
        std::cout << "Dungeon size: width and height, optionally a seed\n"
                  << "(width " << Map::MIN_SIDE << "-" << Map::MAX_WIDTH
                  << ", height " << Map::MIN_SIDE << "-" << Map::MAX_HEIGHT
                  << ", e.g. 40 15 7): " << std::flush;
        std::string line;
        int width = 0;
        int height = 0;
        unsigned int seed = 1;
        if (!std::getline(std::cin, line)) {
            input_ended = true;
        } else if (!parse_size(line, width, height, seed)) {
            std::cout << "Two or three numbers separated by spaces are needed.\n\n";
        } else if (!game.start_random(width, height, seed)) {
            std::cout << "Level not built: invalid size.\n\n";
        } else {
            created = true;
        }
    }
    return created;
}

// Строку — в слова. std::string растёт сам, поэтому длина строки ничем
// не ограничена и проверять её, как в версии на C, не нужно.
std::vector<std::string> split(const std::string& line) {
    std::istringstream stream(line);
    std::vector<std::string> words;
    std::string word;
    while (stream >> word) {
        words.push_back(word);
    }
    return words;
}

void play(Game& game) {
    std::cout << '\n';
    game.draw();
    std::cout << '\n';
    Game::print_commands();

    bool running = true;
    while (running) {
        std::cout << "\nTurn " << game.turns() << " > " << std::flush;
        std::string line;
        if (!std::getline(std::cin, line)) {
            running = false;   // конец ввода
        } else {
            std::vector<std::string> words = split(line);
            if (!words.empty()) {
                // В нижний регистр — только название команды. Аргументы
                // переводятся там, где их сравнивают с ключевыми словами:
                // имя файла уровня менять нельзя.
                for (char& c : words[0]) {
                    if (c >= 'A' && c <= 'Z') {
                        c = static_cast<char>(c - 'A' + 'a');
                    }
                }
                running = game.execute(words);
            }
        }
    }
    std::cout << "\nGame over. Turns made: " << game.turns() << '\n';
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc > 2) {
        std::cerr << "Usage: " << argv[0] << " [level file]\n";
        return 1;
    }
    std::cout << "Welcome to the dungeon!\n\n";

    // Игра занимает около 15 КБ: клетки карты и пометки поиска — массивы
    // внутри объектов. Для стека это немного.
    Game game;
    if (argc == 2) {
        // Ошибка файла, заданного при запуске, — ошибка запуска: в stderr и код 1.
        int bad_line = 0;
        LoadStatus status = game.start_from_file(argv[1], bad_line);
        if (status != LoadStatus::ok) {
            std::cerr << "Level " << argv[1] << " not loaded: " << Map::status_text(status);
            if (bad_line > 0) {
                std::cerr << " (line " << bad_line << ")";
            }
            std::cerr << ".\n";
            std::cout << "\nGoodbye.\n";
            return 1;
        }
    } else if (!ask_random_level(game)) {
        std::cout << "\nGoodbye.\n";
        return 0;   // конец ввода при вопросе о размере — не ошибка
    }
    play(game);
    return 0;
}
