#ifndef MAP_H
#define MAP_H

#include "status.h"

/* Практика 8: функции, которые могут не справиться, возвращают Status.

   Практика 7: карта произвольного размера в динамической памяти.

   Клетки лежат одним непрерывным блоком из width * height символов,
   строка за строкой: сначала вся строка 0, затем вся строка 1 и так далее.
   Клетка в строке row и столбце col лежит по номеру row * width + col.

   Указатель на блок и размеры всегда нужны вместе, поэтому они собраны
   в структуру. */

#define MAP_MIN_SIDE   5
#define MAP_MAX_WIDTH  120
#define MAP_MAX_HEIGHT 60

#define TILE_WALL  '#'
#define TILE_FLOOR '.'
#define TILE_GOLD  '$'
#define TILE_EXIT  '>'

typedef struct {
    char *cells; /* width * height символов; NULL, если карты нет */
    int width;
    int height;
} Map;

/* Выделяет память под карту width x height, заполненную полом.
   STATUS_BAD_SIZE, если размеры вне допустимых; STATUS_NO_MEMORY, если
   памяти не хватило. При неудаче map->cells равен NULL. */
Status map_create(Map *map, int width, int height);

/* Заполняет карту случайным уровнем: стены, золото, выход. */
void map_generate(Map *map, unsigned int seed);

/* Освобождает память и обнуляет указатель: повторный вызов безопасен. */
void map_destroy(Map *map);

/* Номер клетки в блоке. Вычисляется только здесь. */
int map_index(const Map *map, int row, int col);

int map_inside(const Map *map, int x, int y);
char map_at(const Map *map, int x, int y);
void map_set(Map *map, int x, int y, char tile);
int map_cell_info(const Map *map, int x, int y, char *out_tile, int *out_walkable);

/* Подходит ли символ для клетки карты. */
int map_is_tile(char c);

#endif /* MAP_H */
