#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "map.h"

int map_index(const Map *map, int row, int col) {
    /* Номер строки умножается на ширину, а не на высоту. */
    return row * map->width + col;
}

int map_inside(const Map *map, int x, int y) {
    return map != NULL && map->cells != NULL
        && x >= 0 && y >= 0 && x < map->width && y < map->height;
}

char map_at(const Map *map, int x, int y) {
    if (!map_inside(map, x, y)) {
        return TILE_WALL;
    }
    return map->cells[map_index(map, y, x)];
}

void map_set(Map *map, int x, int y, char tile) {
    if (map_inside(map, x, y)) {
        map->cells[map_index(map, y, x)] = tile;
    }
}

int map_cell_info(const Map *map, int x, int y, char *out_tile, int *out_walkable) {
    if (out_tile == NULL || out_walkable == NULL || !map_inside(map, x, y)) {
        return 0;
    }
    *out_tile = map_at(map, x, y);
    *out_walkable = *out_tile != TILE_WALL;
    return 1;
}

/* Уровень: стены по периметру, случайные перегородки, золото, выход
   в правом нижнем углу. Клетки у старта (1, 1) всегда свободны. */
static void generate(Map *map, unsigned int seed) {
    /* Каждое случайное число берётся отдельной инструкцией: порядок
       вычисления аргументов функции не определён, и с rand() прямо
       в аргументах уровень зависел бы от компилятора. */
    srand(seed);
    for (int y = 0; y < map->height; ++y) {
        for (int x = 0; x < map->width; ++x) {
            int border = x == 0 || y == 0 || x == map->width - 1 || y == map->height - 1;
            map_set(map, x, y, border ? TILE_WALL : TILE_FLOOR);
        }
    }
    int walls = map->width * map->height / 25;
    for (int i = 0; i < walls; ++i) {
        int sx = 1 + rand() % (map->width - 2);
        int sy = 1 + rand() % (map->height - 2);
        int length = 2 + rand() % 6;
        int horizontal = rand() % 2;
        for (int k = 0; k < length; ++k) {
            int wx = horizontal ? sx + k : sx;
            int wy = horizontal ? sy : sy + k;
            if (wx > 2 || wy > 2) {
                map_set(map, wx, wy, TILE_WALL);
            }
        }
    }
    int piles = map->width * map->height / 60 + 1;
    for (int i = 0; i < piles; ++i) {
        int gx = 1 + rand() % (map->width - 2);
        int gy = 1 + rand() % (map->height - 2);
        if (map_at(map, gx, gy) == TILE_FLOOR) {
            map_set(map, gx, gy, TILE_GOLD);
        }
    }
    map_set(map, map->width - 2, map->height - 2, TILE_EXIT);
}

int map_create(Map *map, int width, int height, unsigned int seed) {
    if (map == NULL) {
        return 0;
    }
    map->cells = NULL;
    map->width = 0;
    map->height = 0;

    if (width < MAP_MIN_SIDE || height < MAP_MIN_SIDE
        || width > MAP_MAX_WIDTH || height > MAP_MAX_HEIGHT) {
        return 0;
    }
    /* Произведение проверяется на переполнение до умножения. При наших
       пределах оно невозможно, но проверка стоит рядом с умножением,
       чтобы не исчезнуть, когда пределы поменяют. */
    if ((size_t)width > (size_t)-1 / (size_t)height) {
        return 0;
    }
    map->cells = malloc((size_t)width * (size_t)height);
    if (map->cells == NULL) {
        return 0; /* результат выделения проверяется до использования */
    }
    map->width = width;
    map->height = height;
    generate(map, seed);
    return 1;
}

void map_destroy(Map *map) {
    if (map == NULL) {
        return;
    }
    free(map->cells);
    map->cells = NULL; /* после освобождения указатель обнуляется */
    map->width = 0;
    map->height = 0;
}

void map_draw(const Map *map, int hero_x, int hero_y,
              const int *monster_cells, int monster_count) {
    if (map == NULL || map->cells == NULL) {
        return;
    }
    for (int y = 0; y < map->height; ++y) {
        for (int x = 0; x < map->width; ++x) {
            int cell = map_index(map, y, x);
            char shown = map->cells[cell];
            for (int i = 0; i < monster_count; ++i) {
                if (monster_cells[i] == cell) {
                    shown = 'M';
                }
            }
            if (x == hero_x && y == hero_y) {
                shown = '@';
            }
            putchar(shown);
        }
        putchar('\n');
    }
}
