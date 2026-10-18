#ifndef MAP_H
#define MAP_H

/* Практика 4: карта вынесена в отдельный модуль.

   Сам массив объявлен в map.c и снаружи не виден: остальной код
   обращается к карте только через функции ниже. Индексы по-прежнему
   map[y][x] — строка, затем столбец. */

#define MAP_ROWS 10
#define MAP_COLS 30

#define TILE_WALL  '#'
#define TILE_FLOOR '.'
#define TILE_GOLD  '$'
#define TILE_EXIT  '>'

/* Заполняет карту: стены, золото, выход. */
void map_init(void);

/* Лежит ли клетка внутри карты. */
int map_inside(int x, int y);

/* Символ клетки. За пределами карты возвращает стену: проверка границ
   спрятана здесь, и забыть её снаружи нельзя. */
char map_at(int x, int y);

/* Меняет клетку; за пределами карты ничего не делает. */
void map_set(int x, int y, char tile);

/* Печатает карту, подставляя героя в его клетку. Сама карта не меняется. */
void map_draw(int hero_x, int hero_y);

#endif /* MAP_H */
