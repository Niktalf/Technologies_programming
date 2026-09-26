#include <stdio.h>

#include "map.h"

static const char *const MAP_ROWS[MAP_HEIGHT] = {
    "####################",
    "#........#.........#",
    "#........#.........#",
    "#........#....>....#",
    "#....#####.........#",
    "#....#.............#",
    "#....#....######...#",
    "#.........#....#...#",
    "#.........#....#...#",
    "####################"
};

char map_cell(const int x, const int y)
{
    if (x < 0 || y < 0 || x >= MAP_WIDTH || y >= MAP_HEIGHT) {
        return '#';
    }
    return MAP_ROWS[y][x];
}

void map_draw(const int hero_x, const int hero_y)
{
    for (int y = 0; y < MAP_HEIGHT; ++y) {
        for (int x = 0; x < MAP_WIDTH; ++x) {
            if (x == hero_x && y == hero_y) {
                putchar('@');
            } else {
                putchar(map_cell(x, y));
            }
        }
        putchar('\n');
    }
}
