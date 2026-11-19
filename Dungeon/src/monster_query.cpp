#include "monster_query.hpp"

#include <algorithm>
#include <cstdlib>
#include <numeric>

#include "game.hpp"
#include "monster.hpp"

MonsterPredicate is_alive() {
    return [](const Monster& monster) { return monster.alive(); };
}

MonsterPredicate near_hero(const Game& game, int radius) {
    return [&game, radius](const Monster& monster) {
        int distance = std::abs(game.hero().x() - monster.x())
                       + std::abs(game.hero().y() - monster.y());
        return distance <= radius;
    };
}

MonsterPredicate both(MonsterPredicate first, MonsterPredicate second) {
    return [first, second](const Monster& monster) { return first(monster) && second(monster); };
}

// reduce: std::accumulate складывает, начиная с 0L. Литерал 0 был бы int,
// и сумма копилась бы в int; 0L делает её long — тип начального значения
// задаёт тип результата.
long total_health(const std::vector<const Monster*>& monsters) {
    return std::accumulate(monsters.begin(), monsters.end(), 0L,
                           [](long sum, const Monster* monster) { return sum + monster->hp(); });
}

long total_attack(const std::vector<const Monster*>& monsters) {
    return std::accumulate(monsters.begin(), monsters.end(), 0L,
                           [](long sum, const Monster* monster) {
                               return sum + monster->attack();
                           });
}

// map: std::transform пишет символ каждого монстра в новый вектор того же
// размера. symbol() — виртуальный вызов: у каждого вида свой символ.
std::vector<char> symbols(const std::vector<const Monster*>& monsters) {
    std::vector<char> result(monsters.size());
    std::transform(monsters.begin(), monsters.end(), result.begin(),
                   [](const Monster* monster) { return monster->symbol(); });
    return result;
}
