#include "monster_list.hpp"

#include <iostream>

namespace {

// Сколько монстров показывает print: длинный список не нужен.
constexpr int SHOWN = 20;

} // namespace

void MonsterList::add(int x, int y) {
    monsters_.push_back(Monster::at(x, y));   // монстр копируется в конец вектора
}

void MonsterList::remove(int index) {
    if (index < 0 || index >= count()) {
        return;
    }
    monsters_[static_cast<std::size_t>(index)] = monsters_.back();
    monsters_.pop_back();
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
}
