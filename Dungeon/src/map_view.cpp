#include "map_view.hpp"

#include <iostream>

#include "hero.hpp"
#include "map.hpp"
#include "monster_list.hpp"

void MapView::show(const Map& map, const Hero& hero, const MonsterList& monsters) const {
    for (int y = 0; y < map.height(); ++y) {
        for (int x = 0; x < map.width(); ++x) {
            char shown = map.tile(x, y);
            if (monsters.find(x, y) >= 0) {
                shown = 'M';
            }
            if (hero.x() == x && hero.y() == y) {
                shown = '@';
            }
            std::cout << shown;
        }
        std::cout << '\n';
    }
}
