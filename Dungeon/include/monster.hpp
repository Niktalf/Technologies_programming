#pragma once

// Монстр: координаты и здоровье. На практике 8 это была структура
// с открытыми полями, и любой код мог, например, записать ей отрицательное
// здоровье. Здесь поля закрыты, а меняются они только методами.
class Monster {
public:
    static constexpr int START_HP = 8;

    // Монстр с полным здоровьем в клетке (x, y). Конструкторов пока нет,
    // поэтому монстра создаёт статический метод: Monster::at(3, 4).
    static Monster at(int x, int y) {
        Monster monster;
        monster.x_ = x;
        monster.y_ = y;
        return monster;
    }

    int x() const { return x_; }
    int y() const { return y_; }
    int hp() const { return hp_; }
    bool alive() const { return hp_ > 0; }

    // Удар: здоровье уменьшается и не уходит ниже нуля.
    void take_damage(int amount) {
        hp_ = amount >= hp_ ? 0 : hp_ - amount;
    }

private:
    int x_ = 0;
    int y_ = 0;
    int hp_ = START_HP;
};
