#include "exit_search.hpp"

bool ExitSearch::run(const Map& map, int x, int y) {
    for (int row = 0; row < map.height(); ++row) {
        for (int col = 0; col < map.width(); ++col) {
            visited_[row][col] = false;
        }
    }
    cells_ = 0;
    depth_ = 0;
    visit(map, x, y, 1);

    // Есть ли среди посещённых клеток выход.
    bool found = false;
    for (int row = 0; row < map.height() && !found; ++row) {
        for (int col = 0; col < map.width() && !found; ++col) {
            found = visited_[row][col] && map.tile(col, row) == Map::EXIT;
        }
    }
    return found;
}

// Три базовых случая проверяются до рекурсивных вызовов: клетка за границей,
// стена, уже посещённая клетка. Границы — первыми, иначе пометка клетки
// за краем прочиталась бы из чужой памяти.
void ExitSearch::visit(const Map& map, int x, int y, int depth) {
    if (!map.inside(x, y) || !map.walkable(x, y) || visited_[y][x]) {
        return;
    }
    visited_[y][x] = true;
    ++cells_;
    if (depth > depth_) {
        depth_ = depth;
    }
    visit(map, x + 1, y, depth + 1);
    visit(map, x - 1, y, depth + 1);
    visit(map, x, y + 1, depth + 1);
    visit(map, x, y - 1, depth + 1);
}
