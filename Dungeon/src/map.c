#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "map.h"

static char cells[MAP_HEIGHT][MAP_WIDTH];

static const char *const LEVEL[MAP_HEIGHT] = {
    "########################################",
    "#.........#..................#.........#",
    "#.........#..................#.........#",
    "#.........#.......######.....#....>...$#",
    "#.....#####.......#....#...............#",
    "#.....#....$......#....#.....#....$....#",
    "#.....#....########....#######.........#",
    "#....$....#.......$....................#",
    "#.........#.....#####..........#########",
    "#######...#.....#...#..........#.......#",
    "#.........#.........#..........#.......#",
    "########################################"
};

void map_init() {
    for (int y = 0; y < MAP_HEIGHT; ++y) {
        memcpy(cells[y], LEVEL[y], MAP_WIDTH);
    }
}

char map_at(const int x, const int y) {
    if (x < 0 || y < 0 || x >= MAP_WIDTH || y >= MAP_HEIGHT) {
        return TILE_WALL;
    }
    return cells[y][x];
}

int map_walkable(const int x, const int y) {
    return map_at(x, y) != TILE_WALL;
}

void map_set(const int x, const int y, const char tile) {
    if (x < 0 || y < 0 || x >= MAP_WIDTH || y >= MAP_HEIGHT) {
        return;
    }
    cells[y][x] = tile;
}

int map_find_free(const int from_x, const int from_y, int *out_x, int *out_y) {
    if (out_x == NULL || out_y == NULL) {
        return 0;
    }
    for (int radius = 0; radius < MAP_WIDTH + MAP_HEIGHT; ++radius) {
        for (int dy = -radius; dy <= radius; ++dy) {
            for (int dx = -radius; dx <= radius; ++dx) {
                if (dx != -radius && dx != radius && dy != -radius && dy != radius) {
                    continue;
                }
                const int x = from_x + dx;
                const int y = from_y + dy;
                if (map_walkable(x, y)) {
                    *out_x = x;
                    *out_y = y;
                    return 1;
                }
            }
        }
    }
    return 0;
}

void map_draw(const int hero_x, const int hero_y) {
    for (int y = 0; y < MAP_HEIGHT; ++y) {
        for (int x = 0; x < MAP_WIDTH; ++x) {
            if (x == hero_x && y == hero_y) {
                putchar('@');
            } else {
                putchar(map_at(x, y));
            }
        }
        putchar('\n');
    }
}
