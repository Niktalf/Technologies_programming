#include "monster.hpp"

// Статическое поле объявлено в классе, а определяется здесь, один раз
// на всю программу.
int Monster::created_count_ = 0;

Monster::Monster(int x, int y)
    : x_(x), y_(y) {
    ++created_count_;
}

void Monster::take_damage(int amount) {
    if (amount <= 0) {
        return;
    }
    hp_ = amount >= hp_ ? 0 : hp_ - amount;
}
