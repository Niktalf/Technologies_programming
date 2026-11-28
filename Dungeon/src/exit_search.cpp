#include "exit_search.hpp"

bool ExitSearch::run(const Map& map, int x, int y) {
    // assign задаёт новый размер и обнуляет все пометки. Если вектор уже
    // был нужного размера, память заново не выделяется.
    width_ = map.width();
    std::size_t count = static_cast<std::size_t>(width_) * static_cast<std::size_t>(map.height());
    visited_.assign(count, 0);
    cells_ = 0;
    depth_ = 0;
    found_ = false;
    // Практика 14: выход отмечается в самом обходе, когда до него дошли.
    // На практике 13 после обхода был ещё цикл по всем клеткам с флагом
    // «нашли» — ручное накопление, которое здесь просто не нужно.
    visit(map, x, y, 1);
    return found_;
}

// Три базовых случая проверяются до рекурсивных вызовов: клетка за границей,
// стена, уже посещённая клетка. Границы — первыми, иначе пометка клетки
// за краем прочиталась бы из чужой памяти.
void ExitSearch::visit(const Map& map, int x, int y, int depth) {
    if (!map.inside(x, y) || !map.walkable(x, y) || visited_[cell(x, y)] != 0) {
        return;
    }
    visited_[cell(x, y)] = 1;
    ++cells_;
    if (map.tile(x, y) == Map::EXIT) {
        found_ = true;
    }
    if (depth > depth_) {
        depth_ = depth;
    }
    visit(map, x + 1, y, depth + 1);
    visit(map, x - 1, y, depth + 1);
    visit(map, x, y + 1, depth + 1);
    visit(map, x, y - 1, depth + 1);
}
