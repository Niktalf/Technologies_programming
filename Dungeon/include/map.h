#ifndef MAP_H
#define MAP_H

#define MAP_WIDTH  40
#define MAP_HEIGHT 12

#define TILE_WALL   '#'
#define TILE_FLOOR  '.'
#define TILE_STAIRS '>'

void map_init();

char map_at(int x, int y);

int map_walkable(int x, int y);

void map_draw(int hero_x, int hero_y);

#endif // MAP_H
