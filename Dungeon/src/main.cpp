#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "game.hpp"

// Практика 10: карта произвольного размера или из файла; память под клетки
// выделяет конструктор, освобождает деструктор. Явного освобождения памяти
// вне деструкторов в программе нет, кроме опыта 3 в demo, где объект
// нарочно создан в куче.
//
//   ./dungeon                    — спросит размер и построит случайный уровень
//   ./dungeon levels/cave.txt    — уровень из файла
//   ./dungeon demo               — опыты со временем жизни объектов
//
// main только читает строки и передаёт их игре. Ни карта, ни герой ввода
// не читают.

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

// Строку — в слова.
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

// Спрашивает размер случайного уровня, пока ответ негоден, строит уровень
// и играет на нём. Возвращает код завершения программы.
//
// Игра создаётся внутри блока try: если размер неверный, конструктор
// бросает исключение, и объекта нет — освобождать нечего. Если всё удалось,
// игра живёт до конца блока, а при выходе из него деструктор карты
// освобождает клетки сам, на каком бы пути ни закончилась игра.
int play_random_level() {
    bool played = false;
    bool input_ended = false;
    while (!played && !input_ended) {
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
        } else {
            try {
                Game game(width, height, seed);
                play(game);
                played = true;
            } catch (const std::exception&) {
                // invalid_argument — неверный размер, runtime_error — проходимый
                // уровень не получился. Для игрока это одно и то же.
                std::cout << "Level not built: invalid size.\n\n";
            }
        }
    }
    if (input_ended) {
        std::cout << "\nGoodbye.\n";
    }
    return 0;   // конец ввода при вопросе о размере — не ошибка
}

// Уровень из файла. Ошибка файла, заданного при запуске, — ошибка запуска:
// в stderr и код 1.
int play_level_file(const char* path) {
    try {
        Game game(path);
        play(game);
    } catch (const LevelError& error) {
        // Сюда исключение пришло из конструктора карты: объекта game нет,
        // и деструкторы не вызывались — освобождать нечего.
        std::cerr << "Level " << path << " not loaded: " << error.what() << ".\n";
        std::cout << "\nGoodbye.\n";
        return 1;
    }
    return 0;
}

// Опыты со временем жизни. Map::trace включает сообщения из конструкторов,
// деструктора и перемещений карты, Map::live_count показывает, сколько карт
// владеют клетками.
void demo() {
    Map::trace = true;

    std::cout << "1. A stack object is destroyed when the block ends:\n";
    {
        Map map(8, 6);
        std::cout << "  inside the block, maps: " << Map::live_count() << '\n';
    }   // здесь вызывается деструктор map
    std::cout << "  after the block, maps: " << Map::live_count() << "\n\n";

    // Объекты на стеке создаются в порядке объявления и разрушаются
    // в обратном: последний положенный на стек снимается первым.
    std::cout << "2. Two objects in one block: created in order,\n";
    std::cout << "   destroyed in reverse - the last created goes first:\n";
    {
        Map first(6, 5);
        Map second(7, 5);
    }
    std::cout << '\n';

    // Объект в куче живёт, пока его не удалят delete: выход из блока
    // его не касается. Забытый delete — утечка, деструктор не вызовется никогда.
    std::cout << "3. A heap object lives until it is deleted explicitly:\n";
    Map* heap = nullptr;
    {
        heap = new Map(10, 5);
    }
    std::cout << "  the block has ended, but the heap map is alive: maps "
              << Map::live_count() << '\n';
    delete heap;   // delete вызывает деструктор и освобождает память самого объекта
    heap = nullptr;
    std::cout << "  after delete, maps: " << Map::live_count() << "\n\n";

    std::cout << "4. Levels from files. On error no object is created and no memory is lost:\n";
    const char* const files[] = {
        "levels/cave.txt", "levels/windows.txt", "levels/empty.txt", "levels/ragged.txt",
        "levels/nohero.txt", "levels/badchar.txt", "levels/small.txt", "levels/wide.txt",
        "levels/missing.txt"
    };
    for (const char* path : files) {
        try {
            MonsterList monsters;
            Map map(path, monsters);
            std::cout << "    " << map.width() << " x " << map.height() << ", start ("
                      << map.start_x() << ", " << map.start_y() << "), monsters "
                      << monsters.count() << '\n';
        } catch (const LevelError& error) {
            std::cout << "  " << path << ": " << error.what() << '\n';
        }
    }
    std::cout << "  maps after all attempts: " << Map::live_count() << "\n\n";

    // Перемещение передаёт клетки другому объекту: новой памяти нет,
    // клетки не копируются. Источник остаётся пустой картой 0 на 0.
    std::cout << "5. Moving: the cells change owner, nothing is copied:\n";
    {
        Map first(6, 5);
        Map second = std::move(first);   // перемещающий конструктор
        std::cout << "  the source after the move: " << first.width() << " x "
                  << first.height() << ", maps: " << Map::live_count() << '\n';
        Map third(7, 5);
        third = std::move(second);       // перемещающее присваивание: 7x5 освобождается
        std::cout << "  after the assignment, maps: " << Map::live_count() << '\n';
    }   // клетками владеет только third — и освобождает их только его деструктор
    std::cout << "  after the block, maps: " << Map::live_count() << "\n\n";

    std::cout << "6. Hero: default constructor and constructor with parameters:\n";
    Hero by_default;
    Hero custom(3, 4, 50);
    by_default.print_status();
    custom.print_status();

    Map::trace = false;
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc > 2) {
        std::cerr << "Usage: " << argv[0] << " [level file | demo]\n";
        return 1;
    }
    if (argc == 2 && std::string(argv[1]) == "demo") {
        demo();
        std::cout << "\nEnd of demo, maps: " << Map::live_count() << '\n';
        return 0;
    }
    std::cout << "Welcome to the dungeon!\n\n";
    if (argc == 2) {
        return play_level_file(argv[1]);
    }
    return play_random_level();
}
