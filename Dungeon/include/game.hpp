#pragma once

#include <string>
#include <vector>

#include "hero.hpp"
#include "level_generator.hpp"
#include "map.hpp"
#include "map_view.hpp"
#include "monster_list.hpp"

// Игра: карта, монстры, герой и счётчик ходов вместе.
//
// Практика 12: игра — композиция. Она владеет картой, героем и списком
// монстров, а список владеет монстрами: игра кончилась — исчезло всё.
// С картой герой и монстры связаны иначе — ассоциацией: шаг героя получает
// карту параметром и спрашивает её, но не владеет ею.
//
// После каждого хода игрока (шаг или удар) ходят все существа: герой —
// своим act (яд, огонь), монстры — каждый своим act. Для этого игра
// открывает существам несколько вопросов о мире: где герой, свободна ли
// клетка, — и одно действие: ударить героя. На практике 8 это была
// структура Game, и обработчики команд получали её адрес. Здесь обработчики —
// закрытые методы: им не нужно передавать игру параметром, она у них есть
// всегда — это объект, через который они вызваны.
//
// Игра не читает ввод сама: она получает уже разбитую на слова команду.
// Чтение строк — дело главного цикла в main.cpp.
//
// Инвариант игры — то, что связывает её части и чего не может проверить
// ни одна часть в одиночку:
//   * герой стоит внутри карты на клетке, которая не стена;
//   * каждый монстр стоит внутри карты на клетке, которая не стена;
//   * ни один монстр не стоит в клетке героя, и никакие два монстра
//     не стоят в одной клетке;
//   * turns() не убывает.
// Конструкторы ставят героя на старт карты, а он по инварианту карты
// проходим. Монстров селит spawn_monsters только на пол и не к герою,
// уровень из файла — только там, где в файле стояла буква M. Герой ходит
// через move, который не пускает в стену и к монстру.
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

    // Что видят существа во время своего хода.
    const Map& map() const { return map_; }
    const Hero& hero() const { return hero_; }

    // Свободна ли клетка для шага монстра: внутри карты, не стена, не герой
    // и не другой монстр. На этой проверке держится инвариант игры.
    bool free_cell(int x, int y) const;

    // Монстр бьёт героя: урон по силе удара монстра и сообщение.
    void hit_hero(const Monster& attacker);

    void draw() const;
    static void print_commands();

private:
    bool command_move(Hero::Direction direction);
    bool command_attack(std::string* argument);
    void command_rule(const std::string* argument);
    void command_summary() const;
    void command_cell() const;
    void command_check();
    void command_level(const std::string* path);
    void command_spawn(const std::string* argument);
    void command_use(const std::string* argument);
    bool command_hit(const std::string* argument);
    void command_state(const std::string& command, std::string* argument);

    // Селит до n монстров в свободные клетки. Возвращает, сколько поселено.
    int spawn_monsters(int n);

    // Конец хода игрока: ход героя (яд, огонь), затем ход каждого монстра.
    // Возвращает false, если герой погиб.
    bool end_turn();

    // Поля создаются в порядке объявления: монстры раньше карты, потому что
    // конструктор карты из файла заполняет список монстров, а герой после
    // карты, потому что встаёт на её стартовую клетку.
    MonsterList monsters_;
    Map map_;
    Hero hero_;
    LevelGenerator generator_;
    ExitSearch search_;
    MapView view_;
    int turns_ = 0;
};
