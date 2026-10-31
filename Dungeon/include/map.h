#ifndef MAP_H
#define MAP_H

/* Практика 7: карта произвольного размера в динамической памяти.

   Клетки лежат одним непрерывным блоком из width * height символов,
   строка за строкой: сначала вся строка 0, затем вся строка 1 и так далее.
   Клетка в строке row и столбце col лежит по номеру row * width + col.

   Указатель на блок и размеры всегда нужны вместе, поэтому они собраны
   в маленькую структуру-контейнер. Структуры для героя, монстра и всей
   игры появятся на практике 8. */

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

/* Выделяет память под карту и заполняет её уровнем. Возвращает 1 при
   успехе и 0, если размеры вне допустимых или памяти не хватило.
   При неудаче map->cells равен NULL. */
int map_create(Map *map, int width, int height, unsigned int seed);

/* Освобождает память и обнуляет указатель: повторный вызов безопасен. */
void map_destroy(Map *map);

/* Номер клетки в блоке. Вычисляется только здесь. */
int map_index(const Map *map, int row, int col);

int map_inside(const Map *map, int x, int y);
char map_at(const Map *map, int x, int y);
void map_set(Map *map, int x, int y, char tile);
int map_cell_info(const Map *map, int x, int y, char *out_tile, int *out_walkable);

/* Печатает карту. Героя и монстров подставляет поверх клеток. */
void map_draw(const Map *map, int hero_x, int hero_y,
              const int *monster_cells, int monster_count);

#endif /* MAP_H */
