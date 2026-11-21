#include "creature.hpp"

#include <stdexcept>

int Creature::live_count_ = 0;

Creature::Creature(int x, int y, int hp_max)
    : x_(x), y_(y), hp_max_(hp_max), hp_(hp_max) {
    if (hp_max <= 0) {
        throw std::invalid_argument("creature health must be positive");
    }
    ++live_count_;
}

Creature::Creature(const Creature& other)
    : x_(other.x_), y_(other.y_), hp_max_(other.hp_max_), hp_(other.hp_) {
    ++live_count_;
}

Creature::~Creature() {
    --live_count_;
}

void Creature::take_damage(int amount) {
    if (amount <= 0) {
        return;
    }
    hp_ = amount >= hp_ ? 0 : hp_ - amount;
}

void Creature::heal(int amount) {
    if (amount <= 0) {
        return;
    }
    hp_ = amount >= hp_max_ - hp_ ? hp_max_ : hp_ + amount;
}

void Creature::move_to(int x, int y) {
    x_ = x;
    y_ = y;
}
