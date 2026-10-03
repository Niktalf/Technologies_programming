#include <stdio.h>
#include <string.h>

#include "map.h"

static char cells[MAP_HEIGHT][MAP_WIDTH];

static const char *const LEVEL[MAP_HEIGHT] = {
    "########################################",
    "#.........#..................#.........#",
    "#.........#..................#.........#",
    "#.........#.......######.....#....>....#",
    "#.....#####.......#....#...............#",
    "#.....#...........#....#.....#.........#",
    "#.....#....########....#######.........#",
    "#.........#............................#",
    "#.........#.....#####..........#########",
    "#######...#.....#...#..........#.......#",
    "#.........#.........#..........#.......#",
    "########################################"
};

void map_init()
{
    for (int y = 0; y < MAP_HEIGHT; ++y) {
        memcpy(cells[y], LEVEL[y], MAP_WIDTH);
    }
}

char map_at(const int x, const int y)
{
    if (x < 0 || y < 0 || x >= MAP_WIDTH || y >= MAP_HEIGHT) {
        return TILE_WALL;
    }
    return cells[y][x];
}

int map_walkable(const int x, const int y)
{
    return map_at(x, y) != TILE_WALL;
}

void map_draw(const int hero_x, const int hero_y)
{
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
