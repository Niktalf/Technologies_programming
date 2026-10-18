#include <stddef.h>

#include "hero.h"

int hero_move(char map[][MAP_COLS], int *x, int *y, int direction) {
    static const int DX[4] = { 0, 0, 1, -1 };
    static const int DY[4] = { -1, 1, 0, 0 };

    if (map == NULL || x == NULL || y == NULL || direction < 0 || direction > 3) {
        return 0;
    }
    int nx = *x + DX[direction];
    int ny = *y + DY[direction];

    /* Клетка вне карты или стена — шага нет. map_cell_info отвечает
       на оба вопроса сразу и сама проверяет границы. */
    char tile = TILE_WALL;
    int walkable = 0;
    if (!map_cell_info(map, nx, ny, &tile, &walkable) || !walkable) {
        return 0;
    }
    /* Пишем по адресу: меняется переменная вызывающего, а не копия. */
    *x = nx;
    *y = ny;
    return 1;
}

void hero_damage(int *hp, int amount) {
    if (hp == NULL) {
        return;
    }
    /* Скобки не нужны, но полезно помнить: *hp - amount — это значение
       по адресу минус урон, а *hp++ сдвинул бы сам указатель. */
    *hp = *hp - amount;
    if (*hp < 0) {
        *hp = 0;
    }
}

int hero_pick_gold(char map[][MAP_COLS], int x, int y, unsigned int *gold) {
    if (map == NULL || gold == NULL || map_at(map, x, y) != TILE_GOLD) {
        return 0;
    }
    *gold = *gold + GOLD_PER_PILE;
    map_set(map, x, y, TILE_FLOOR);
    return 1;
}
