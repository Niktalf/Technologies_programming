#include "map_view.hpp"

#include <iostream>

#include "hero.hpp"
#include "map.hpp"
#include "monster_list.hpp"

void MapView::show(const Map& map, const Hero& hero, const MonsterList& monsters) const {
    for (int y = 0; y < map.height(); ++y) {
        for (int x = 0; x < map.width(); ++x) {
            char shown = map.tile(x, y);
            int monster = monsters.find(x, y);
            if (monster >= 0) {
                // Виртуальный вызов: у каждого вида свой символ, и отрисовка
                // не знает, какие виды бывают.
                shown = monsters.at(monster).symbol();
            }
            if (hero.x() == x && hero.y() == y) {
                shown = hero.symbol();
            }
            std::cout << shown;
        }
        std::cout << '\n';
    }
}
