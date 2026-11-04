#pragma once

#include <memory>

#include "creature.hpp"

// Монстр — существо, которое ходит само и бьёт героя. Тоже абстрактный
// класс: act остаётся чисто виртуальным, и «просто монстра» не бывает.
// Здесь — то, что общее у всех монстров: сила удара, счётчик созданных
// и два шага поведения, из которых наследники собирают свой ход.
//
// Инвариант: здоровье — по инварианту Creature; attack() > 0.
//
// Что монстр стоит не в стене и не в чужой клетке — инвариант не монстра,
// а игры. Монстр сохраняет его сам: шагает только туда, где игра
// подтвердила, что клетка свободна (Game::free_cell).
class Monster : public Creature {
public:
    int attack() const { return attack_; }

    // Сколько монстров создано с начала работы программы (практика 11).
    // Счётчик растёт в конструкторе Monster, а его вызывает конструктор
    // каждого наследника — поэтому считаются монстры всех видов.
    static int created_count() { return created_count_; }

protected:
    Monster(int x, int y, int hp_max, int attack);

    // Стоит ли монстр вплотную к герою: по горизонтали или вертикали.
    bool next_to_hero(const Game& game) const;

    // Шаг на (dx, dy), если клетка свободна. Возвращает, сделан ли шаг.
    bool step(const Game& game, int dx, int dy);

private:
    int attack_;

    static int created_count_;
};

// Гоблин: увидел героя — идёт к нему, подошёл вплотную — бьёт.
class Goblin : public Monster {
public:
    static constexpr int HP = 8;
    static constexpr int ATTACK = 3;
    static constexpr int SIGHT = 8;   // дальше этого героя не видит

    Goblin(int x, int y);

    void act(Game& game) override;
    char symbol() const override { return 'g'; }
    const char* name() const override { return "goblin"; }
};

// Летучая мышь: порхает куда придётся. Налетела на героя — кусает.
class Bat : public Monster {
public:
    static constexpr int HP = 4;
    static constexpr int ATTACK = 1;

    Bat(int x, int y);

    void act(Game& game) override;
    char symbol() const override { return 'b'; }
    const char* name() const override { return "bat"; }
};

// Скелет: стоит на месте, как страж. Бьёт, только если герой рядом.
class Skeleton : public Monster {
public:
    static constexpr int HP = 12;
    static constexpr int ATTACK = 4;

    Skeleton(int x, int y);

    void act(Game& game) override;
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
