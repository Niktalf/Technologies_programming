#include <stdio.h>

#include "map.h"

/* Массив карты. static: виден только в этом файле. */
static char map[MAP_ROWS][MAP_COLS];

void map_init(void) {
    for (int y = 0; y < MAP_ROWS; ++y) {
        for (int x = 0; x < MAP_COLS; ++x) {
            if (y == 0 || y == MAP_ROWS - 1 || x == 0 || x == MAP_COLS - 1) {
                map[y][x] = TILE_WALL;
            } else {
                map[y][x] = TILE_FLOOR;
            }
        }
    }
    /* Внутренние стены: вертикальная с проходом внизу и горизонтальная
       с проходом справа. */
    for (int y = 1; y < MAP_ROWS - 3; ++y) {
        map[y][9] = TILE_WALL;
    }
    for (int x = 12; x < MAP_COLS - 4; ++x) {
        map[5][x] = TILE_WALL;
    }
    map[2][5] = TILE_GOLD;
    map[3][20] = TILE_GOLD;
    map[7][25] = TILE_GOLD;
    map[8][2] = TILE_GOLD;
    map[3][27] = TILE_EXIT;
}

int map_inside(int x, int y) {
    return x >= 0 && y >= 0 && x < MAP_COLS && y < MAP_ROWS;
}

char map_at(int x, int y) {
    if (!map_inside(x, y)) {
        return TILE_WALL;
    }
    return map[y][x];
}

void map_set(int x, int y, char tile) {
    if (map_inside(x, y)) {
        map[y][x] = tile;
    }
}

void map_draw(int hero_x, int hero_y) {
    for (int y = 0; y < MAP_ROWS; ++y) {
        for (int x = 0; x < MAP_COLS; ++x) {
            putchar(x == hero_x && y == hero_y ? '@' : map[y][x]);
        }
        putchar('\n');
    }
}
