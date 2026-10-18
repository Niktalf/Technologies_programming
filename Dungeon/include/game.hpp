#pragma once

#include <string>
#include <vector>

#include "hero.hpp"
#include "level_generator.hpp"
#include "map.hpp"
#include "monster_list.hpp"

// Игра: карта, монстры, герой и счётчик ходов вместе. На практике 8 это была
// структура Game, и обработчики команд получали её адрес. Здесь обработчики —
// закрытые методы: им не нужно передавать игру параметром, она у них есть
// всегда — это объект, через который они вызваны.
//
// Игра не читает ввод сама: она получает уже разбитую на слова команду.
// Чтение строк — дело главного цикла в main.cpp.
//
// Практика 10: игра создаётся конструктором — сразу с уровнем. Карта
// не копируется, но перемещается, поэтому смена уровня командой level —
// это перемещающее присваивание новой карты на место старой.
class Game {
public:
    // Случайный уровень width x height и монстры — как в версии на C.
    // Неразумный размер — std::invalid_argument, проходимый уровень
    // не получился — std::runtime_error; в обоих случаях игры нет.
    Game(int width, int height, unsigned int seed);

    // Уровень из файла. Ошибка в файле — LevelError, игры нет.
    explicit Game(const std::string& path);

    // Выполняет одну команду. words[0] — название команды в нижнем регистре.
    // Возвращает false, если игра закончилась.
    bool execute(std::vector<std::string>& words);

    int turns() const { return turns_; }

    void draw() const;
    static void print_commands();

private:
    void command_move(Hero::Direction direction);
    void command_attack(std::string* argument);
    void command_cell() const;
    void command_check();
    void command_level(const std::string* path);
    void command_spawn(const std::string* argument);
    void command_use(const std::string* argument);
    bool command_hit(const std::string* argument);
    void command_state(const std::string& command, std::string* argument);

    // Селит до n монстров в свободные клетки. Возвращает, сколько поселено.
    int spawn_monsters(int n);

    // Поля создаются в порядке объявления: монстры раньше карты, потому что
    // конструктор карты из файла заполняет список монстров, а герой после
    // карты, потому что встаёт на её стартовую клетку.
    MonsterList monsters_;
    Map map_;
    Hero hero_;
    LevelGenerator generator_;
    ExitSearch search_;
    int turns_ = 0;
};
