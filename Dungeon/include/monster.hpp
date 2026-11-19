#pragma once

#include <memory>

#include "creature.hpp"
#include "move_rule.hpp"

// Монстр — существо, которое ходит само и бьёт героя. Абстрактный класс:
// symbol и name остаются чисто виртуальными, и «просто монстра» не бывает.
//
// Практика 13: ход у всех монстров один — act написан здесь, в Monster.
// Рядом герой — удар; нет — шаг, который скажет правило rule_. Чем гоблин
// отличается от мыши, теперь записано не в коде act, а в правиле, которое
// он получил при создании. Правило можно сменить во время игры командой rule.
//
// Инвариант: здоровье — по инварианту Creature; attack() > 0.
//
// Что монстр стоит не в стене и не в чужой клетке — инвариант не монстра,
// а игры. Монстр сохраняет его сам: шагает только туда, где игра
// подтвердила, что клетка свободна (Game::free_cell).
class Monster : public Creature {
public:
    int attack() const { return attack_; }

    // Ход: удар, если герой рядом, иначе шаг по правилу. Ни одного
    // ветвления по виду монстра — их и не может быть: Monster не знает
    // своих наследников.
    void act(Game& game) override;

    // Текущее правило и его замена. Правило — значение: его копируют,
    // присваивают, заворачивают в drunk.
    const MoveRule& rule() const { return rule_; }
    void set_rule(MoveRule rule);

    // Вернуть правило, с которым монстр был создан.
    void reset_rule() { rule_ = native_rule_; }

    // Сколько монстров создано с начала работы программы (практика 11).
    // Счётчик растёт в конструкторе Monster, а его вызывает конструктор
    // каждого наследника — поэтому считаются монстры всех видов.
    static int created_count() { return created_count_; }

protected:
    // rule — правило хода этого вида. Пустое правило —
    // std::invalid_argument: монстр без правила не знал бы, как ходить.
    Monster(int x, int y, int hp_max, int attack, MoveRule rule);

private:
    // Стоит ли монстр вплотную к герою: по горизонтали или вертикали.
    bool next_to_hero(const Game& game) const;

    // Шаг на (dx, dy), если клетка свободна. Возвращает, сделан ли шаг.
    bool step(const Game& game, int dx, int dy);

    int attack_;
    MoveRule native_rule_;   // правило вида — для reset_rule
    MoveRule rule_;          // правило, по которому монстр ходит сейчас

    static int created_count_;
};

// Три вида отличаются числами, символом и правилом хода, которое каждый
// передаёт в конструктор Monster. Своего act у них больше нет.

// Гоблин: увидел героя — идёт к нему (правило chase), вплотную — бьёт.
class Goblin : public Monster {
public:
    static constexpr int HP = 8;
    static constexpr int ATTACK = 3;
    static constexpr int SIGHT = 8;   // дальше этого героя не видит

    Goblin(int x, int y);

    char symbol() const override { return 'g'; }
    const char* name() const override { return "goblin"; }
};

// Летучая мышь: порхает куда придётся (wander). Рядом с героем — кусает.
class Bat : public Monster {
public:
    static constexpr int HP = 4;
    static constexpr int ATTACK = 1;

    Bat(int x, int y);

    char symbol() const override { return 'b'; }
    const char* name() const override { return "bat"; }
};

// Скелет: стоит на месте (stand_still), как страж. Бьёт, если герой рядом.
class Skeleton : public Monster {
public:
    static constexpr int HP = 12;
    static constexpr int ATTACK = 4;

    Skeleton(int x, int y);

    char symbol() const override { return 's'; }
    const char* name() const override { return "skeleton"; }
};

// Монстр по символу из файла уровня: g, b, s; M — гоблин, как в файлах
// практики 8. Для другого символа — пустой указатель.
//
// Это единственное место в программе, где перечислены конкретные виды
// монстров. Новый вид — новый класс и одна строка здесь; игровой цикл,
// список монстров и отрисовка не меняются.
std::unique_ptr<Monster> make_monster(char symbol, int x, int y);

// Случайный символ вида монстра — для spawn и случайного уровня.
char random_monster_symbol();
