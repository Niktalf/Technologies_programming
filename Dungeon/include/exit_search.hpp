#pragma once

#include "map.hpp"

// Поиск выхода: достижим ли он из заданной клетки. Рекурсивный обход,
// как на практике 4. Нужен двоим: генератору уровней (проверить, что
// уровень проходим) и команде check (ответить игроку). Поэтому это
// отдельный класс, а не закрытый метод одного из них.
class ExitSearch {
public:
    // Обходит все клетки, достижимые из (x, y). Возвращает true, если
    // среди них есть выход.
    bool run(const Map& map, int x, int y);

    // Итоги последнего обхода: сколько клеток посещено и наибольшая
    // глубина рекурсии.
    int cells() const { return cells_; }
    int depth() const { return depth_; }

private:
    void visit(const Map& map, int x, int y, int depth);

    // Пометки посещённых клеток — массив наибольшего размера, как и клетки
    // карты на этой практике.
    bool visited_[Map::MAX_HEIGHT][Map::MAX_WIDTH] = {};
    int cells_ = 0;
    int depth_ = 0;
};
