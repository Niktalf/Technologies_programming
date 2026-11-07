#include "game.hpp"

#include <climits>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <utility>

namespace {

// Переводит слово в нижний регистр на месте. Только латиница: русские
// буквы в UTF-8 занимают по два байта, и побайтовый перевод их испортил бы.
void to_lower(std::string& word) {
    for (char& c : word) {
        if (c >= 'A' && c <= 'Z') {
            c = static_cast<char>(c - 'A' + 'a');
        }
    }
}

// Совпадает ли слово с полным названием команды или с сокращением.
bool is_command(const std::string& word, const char* full, const char* abbreviation) {
    return word == full || (abbreviation != nullptr && word == abbreviation);
}

bool direction_from_word(const std::string& word, Hero::Direction& out) {
    if (is_command(word, "north", "n")) {
        out = Hero::Direction::north;
        return true;
    }
    if (is_command(word, "south", "s")) {
        out = Hero::Direction::south;
        return true;
    }
    if (is_command(word, "east", "e")) {
        out = Hero::Direction::east;
        return true;
    }
    if (is_command(word, "west", "w")) {
        out = Hero::Direction::west;
        return true;
    }
    return false;
}

// Направление из слова-аргумента (с переводом в нижний регистр). При ошибке
// печатает сообщение и возвращает false.
bool direction_argument(std::string* word, const char* command, Hero::Direction& out) {
    if (word == nullptr) {
        std::cout << "Where? Example: " << command << " north\n";
        return false;
    }
    to_lower(*word);
    if (!direction_from_word(*word, out)) {
        std::cout << "Unknown direction: " << *word << ". Known: north, south, east, west.\n";
        return false;
    }
    return true;
}

// Неотрицательное число из слова по цифрам, как command_parse_number
// в версии на C. При ошибке печатает то же сообщение и возвращает false.
bool number_argument(const std::string* word, const char* example, int& out) {
    const char* error = nullptr;
    if (word == nullptr || word->empty()) {
        error = "A number is missing";
    }
    int value = 0;
    for (std::size_t i = 0; error == nullptr && i < word->size(); ++i) {
        char c = (*word)[i];
        if (c < '0' || c > '9') {
            error = "Not a number: digits only";
        } else if (value > (INT_MAX - (c - '0')) / 10) {
            // Проверка до умножения: переполнение знакового int — неопределённое поведение.
            error = "The number is too large";
        } else {
            value = value * 10 + (c - '0');
        }
    }
    if (error != nullptr) {
        std::cout << error << ". Example: " << example << '\n';
        return false;
    }
    out = value;
    return true;
}

} // namespace

void Game::print_commands() {
    std::cout << "Commands (short form in parentheses):\n";
    std::cout << "  go <where>, north (n), south (s), east (e), west (w) - step\n";
    std::cout << "  look (l) - map; check (c) - is the exit reachable; cell - what is underfoot\n";
    std::cout << "  status (st), inventory (i), use <number> (u) - use an item\n";
    std::cout << "  attack <where> (a) - hit a monster next to you; monsters (m) - list\n";
    std::cout << "  level <file> - load a level; spawn <N> - add monsters\n";
    std::cout << "  hit <N>, on/off/toggle <state>\n";
    std::cout << "  help (h), quit (q)\n";
}

// map_ создаётся в списке инициализации: если размер неверный, конструктор
// карты бросит исключение, и тело конструктора игры даже не начнётся.
// hero_ — после карты, на стартовой клетке пустой комнаты (1, 1).
Game::Game(int width, int height, unsigned int seed)
    : map_(width, height), hero_(map_.start_x(), map_.start_y()) {
    if (!generator_.generate(map_, seed)) {
        // Исключение из тела конструктора: уже созданные поля — карта,
        // список, генератор — разрушатся сами, и клетки карты освободятся.
        throw std::runtime_error("cannot build a passable level of this size");
    }
    // Монстры селятся сразу после генерации: rand продолжает ту же
    // последовательность, что и в версии на C, и монстры встают в те же клетки.
    spawn_monsters(map_.width() * map_.height() / 100 + 1);
}

Game::Game(const std::string& path)
    : map_(path, monsters_), hero_(map_.start_x(), map_.start_y()) {
}

void Game::draw() const {
    hero_.look(map_, monsters_);
}

int Game::spawn_monsters(int n) {
    int placed = 0;
    int attempts = 0;
    while (placed < n && attempts < n * 20) {
        // Две координаты — двумя инструкциями: порядок вычисления
        // аргументов не определён, а так он один на всех компиляторах.
        int x = 1 + std::rand() % (map_.width() - 2);
        int y = 1 + std::rand() % (map_.height() - 2);
        ++attempts;
        if (map_.tile(x, y) == Map::FLOOR && !(x == hero_.x() && y == hero_.y())
            && monsters_.find(x, y) < 0) {
            monsters_.add(x, y);
            ++placed;
        }
    }
    return placed;
}

// Шаг героя. Монстр в клетке назначения загораживает путь.
void Game::command_move(Hero::Direction direction) {
    int tx = 0;
    int ty = 0;
    hero_.target(direction, tx, ty);
    if (monsters_.find(tx, ty) >= 0) {
        std::cout << "A monster blocks the way. Attack: attack <where>\n";
        return;
    }
    if (!hero_.move(direction, map_)) {
        std::cout << "There is a wall.\n";
        return;
    }
    ++turns_;
    draw();
}

void Game::command_attack(std::string* argument) {
    Hero::Direction direction = Hero::Direction::north;
    if (!direction_argument(argument, "attack", direction)) {
        return;
    }
    int tx = 0;
    int ty = 0;
    hero_.target(direction, tx, ty);
    int index = monsters_.find(tx, ty);
    if (index < 0) {
        std::cout << "Nobody there.\n";
        return;
    }
    ++turns_;
    // Ссылка на элемент списка годится только до следующего изменения
    // списка. Здесь она живёт три строки, до удаления.
    Monster& target = monsters_.at(index);
    target.take_damage(hero_.attack());
    if (target.alive()) {
        std::cout << "Hit! The monster has " << target.hp() << " health left.\n";
        return;
    }
    monsters_.remove(index);
    std::cout << "The monster is defeated. Monsters left: " << monsters_.count() << '\n';
}

void Game::command_cell() const {
    int x = hero_.x();
    int y = hero_.y();
    std::cout << "Cell (" << x << ", " << y << "): symbol '" << map_.tile(x, y) << "', "
              << (map_.walkable(x, y) ? "walkable" : "not walkable") << '\n';
}

void Game::command_check() {
    bool found = search_.run(map_, hero_.x(), hero_.y());
    std::cout << (found ? "The exit is reachable." : "The exit is unreachable.") << '\n';
    std::cout << "Cells visited: " << search_.cells() << ", recursion depth: "
              << search_.depth() << '\n';
}

// Загрузка уровня во время игры. При ошибке карта и монстры не меняются,
// и игра продолжается на прежнем уровне.
void Game::command_level(const std::string* path) {
    if (path == nullptr) {
        std::cout << "Which file? Example: level levels/cave.txt\n";
        return;
    }
    // Новый уровень строится в отдельных объектах. Если файл испорчен,
    // конструктор бросит исключение, новых объектов не будет, а текущие
    // карта и монстры останутся какими были.
    try {
        MonsterList monsters;
        Map map(*path, monsters);
        // Перемещающее присваивание: старые клетки освобождаются, новые
        // переходят к map_ без копирования. После этого map — пустая
        // карта, и её деструктор в конце блока ничего не освободит.
        map_ = std::move(map);
        monsters_ = std::move(monsters);
    } catch (const LevelError& error) {
        std::cout << "Level " << *path << " not loaded: " << error.what()
                  << "\nThe game goes on at the previous level.\n";
        return;
    }
    hero_.place_at(map_.start_x(), map_.start_y());
    std::cout << "Level loaded " << *path << ": " << map_.width() << " x " << map_.height()
              << ", monsters " << monsters_.count() << '\n';
    draw();
}

void Game::command_spawn(const std::string* argument) {
    int number = 0;
    if (number_argument(argument, "spawn 10", number)) {
        std::cout << "Monsters added: " << spawn_monsters(number) << '\n';
    }
}

void Game::command_use(const std::string* argument) {
    int number = 0;
    if (!number_argument(argument, "use 1", number)) {
        return;
    }
    if (number < 1 || number > hero_.item_count()) {
        std::cout << "No item number " << number << " in the bag. Items: "
                  << hero_.item_count() << '\n';
        return;
    }
    std::cout << "The hero uses: " << hero_.item_name(number) << '\n';
    hero_.use_item(number);
    std::cout << "Health: " << hero_.hp() << " / " << hero_.hp_max() << '\n';
}

// Наносит урон из аргумента. Возвращает false, если герой погиб.
bool Game::command_hit(const std::string* argument) {
    int damage = 0;
    if (!number_argument(argument, "hit 7", damage)) {
        return true;
    }
    hero_.take_damage(damage);
    std::cout << "The hero takes " << damage << " damage. Health: " << hero_.hp() << " / "
              << hero_.hp_max() << '\n';
    if (!hero_.alive()) {
        std::cout << "The hero is dead.\n";
        return false;
    }
    return true;
}

// Команды on, off и toggle: ставят, снимают или переключают бит состояния.
void Game::command_state(const std::string& command, std::string* argument) {
    if (argument == nullptr) {
        std::cout << "Which state? Example: " << command << " poisoned\n";
        return;
    }
    to_lower(*argument);
    int number = Hero::state_number(*argument);
    if (number < 0) {
        std::cout << "No state " << *argument << ". Known: poisoned, blinded, invisible,\n";
        std::cout << "petrified, burning, asleep\n";
        return;
    }
    if (command == "on") {
        hero_.set_state(number, true);
    } else if (command == "off") {
        hero_.set_state(number, false);
    } else {
        hero_.toggle_state(number);
    }
    std::cout << "Hero is " << Hero::state_name(number) << ": "
              << (hero_.has_state(number) ? "yes" : "no") << '\n';
}

bool Game::execute(std::vector<std::string>& words) {
    const std::string& name = words[0];
    // Аргумент — указатель на второе слово или nullptr, если его нет:
    // так «аргумента нет» отличается от пустого аргумента.
    std::string* argument = words.size() > 1 ? &words[1] : nullptr;
    Hero::Direction direction = Hero::Direction::north;
    if (direction_from_word(name, direction)) {
        command_move(direction);
    } else if (name == "go") {
        if (direction_argument(argument, "go", direction)) {
            command_move(direction);
        }
    } else if (is_command(name, "attack", "a")) {
        command_attack(argument);
    } else if (is_command(name, "look", "l")) {
        draw();
    } else if (is_command(name, "monsters", "m")) {
        monsters_.print();
    } else if (name == "level") {
        command_level(argument);   // имя файла — без перевода в нижний регистр
    } else if (name == "spawn") {
        command_spawn(argument);
    } else if (name == "cell") {
        command_cell();
    } else if (is_command(name, "check", "c")) {
        command_check();
    } else if (is_command(name, "status", "st")) {
        hero_.print_status();
    } else if (is_command(name, "inventory", "i")) {
        hero_.print_inventory();
    } else if (is_command(name, "use", "u")) {
        command_use(argument);
    } else if (name == "hit") {
        return command_hit(argument);
    } else if (name == "on" || name == "off" || name == "toggle") {
        command_state(name, argument);
    } else if (is_command(name, "help", "h")) {
        print_commands();
    } else if (is_command(name, "quit", "q")) {
        return false;
    } else {
        std::cout << "Unknown command: " << name << ". Type help.\n";
    }
    return true;
}
