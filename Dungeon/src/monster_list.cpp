#include "monster_list.hpp"

#include <iostream>

namespace {

// Сколько монстров показывает print: длинный список не нужен.
constexpr int SHOWN = 20;

} // namespace

bool MonsterList::add(int x, int y) {
    if (find(x, y) >= 0) {
        return false;   // второй монстр в той же клетке нарушил бы инвариант
    }
    // emplace_back создаёт монстра прямо в векторе конструктором Monster(x, y).
    monsters_.emplace_back(x, y);
    return true;
}

int MonsterList::find(int x, int y) const {
    for (int i = 0; i < count(); ++i) {
        const Monster& monster = monsters_[static_cast<std::size_t>(i)];
        if (monster.x() == x && monster.y() == y) {
            return i;
        }
    }
    return -1;
}

int MonsterList::hit(int index, int damage) {
    if (index < 0 || index >= count() || damage < 0) {
        return -1;
    }
    Monster& target = monsters_[static_cast<std::size_t>(index)];
    target.take_damage(damage);
    if (target.alive()) {
        return target.hp();
    }
    // Мёртвый монстр убирается сразу, в том же методе: между ударом
    // и удалением нет момента, когда инвариант нарушен для вызывающего.
    monsters_[static_cast<std::size_t>(index)] = monsters_.back();
    monsters_.pop_back();
    return 0;
}

void MonsterList::print() const {
    std::cout << "Monsters: " << count() << '\n';
    for (int i = 0; i < count() && i < SHOWN; ++i) {
        const Monster& monster = monsters_[static_cast<std::size_t>(i)];
        std::cout << "  " << i + 1 << ". (" << monster.x() << ", " << monster.y()
                  << "), health " << monster.hp() << '\n';
    }
    if (count() > SHOWN) {
        std::cout << "  ... and " << count() - SHOWN << " more\n";
    }
    std::cout << "Monsters created since the start: " << Monster::created_count() << '\n';
}
