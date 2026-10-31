#include <stddef.h>
#include <stdlib.h>

#include "map.h"
#include "reach.h"

/* Рекурсивный обход. Базовых случаев три, и все они проверяются в начале,
   до любого рекурсивного вызова: клетка за границей, клетка-стена, клетка
   уже посещена. Порядок важен: границы проверяются первыми, иначе
   пометка клетки за краем карты прочиталась бы из чужой памяти.

   Массив пометок и счётчики приходят адресами: функция меняет их, и все
   рекурсивные вызовы должны видеть одни и те же пометки, а не свои копии. */
static void visit(const Map *map, char *visited,
                  int x, int y, int depth, int *cells, int *deepest) {
    if (!map_inside(map, x, y)) {
        return;
    }
    if (map_at(map, x, y) == TILE_WALL) {
        return;
    }
    if (visited[map_index(map, y, x)]) {
        return; /* без этой проверки две соседние клетки вызывали бы друг друга вечно */
    }

    visited[map_index(map, y, x)] = 1;
    ++*cells; /* скобки не нужны: ++ применяется к *cells, а не к указателю */
    if (depth > *deepest) {
        *deepest = depth;
    }

    visit(map, visited, x + 1, y, depth + 1, cells, deepest);
    visit(map, visited, x - 1, y, depth + 1, cells, deepest);
    visit(map, visited, x, y + 1, depth + 1, cells, deepest);
    visit(map, visited, x, y - 1, depth + 1, cells, deepest);
}

/* Есть ли среди посещённых клеток выход. */
static int exit_visited(const Map *map, const char *visited) {
    for (int y = 0; y < map->height; ++y) {
        for (int x = 0; x < map->width; ++x) {
            if (visited[map_index(map, y, x)] && map_at(map, x, y) == TILE_EXIT) {
                return 1;
            }
        }
    }
    return 0;
}

Status reach_exit(const Map *map, int start_x, int start_y,
                  int *out_found, int *out_cells, int *out_depth) {
    if (map == NULL || map->cells == NULL || out_found == NULL) {
        return STATUS_BAD_SIZE;
    }
    /* calloc выделяет и сразу обнуляет: каждый запуск начинает с чистых
       пометок. Размер — ровно по карте. */
    char *visited = calloc((size_t)map->width * (size_t)map->height, 1);
    if (visited == NULL) {
        return STATUS_NO_MEMORY;
    }
    int cells = 0;
    int deepest = 0;
    visit(map, visited, start_x, start_y, 1, &cells, &deepest);
    *out_found = exit_visited(map, visited);

    /* Выделили в этой функции — здесь и освобождаем. */
    free(visited);
    if (out_cells != NULL) {
        *out_cells = cells;
    }
    if (out_depth != NULL) {
        *out_depth = deepest;
    }
    return STATUS_OK;
}
