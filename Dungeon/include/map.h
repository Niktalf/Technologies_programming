#ifndef MAP_H
#define MAP_H

/* Практика 5: массив карты больше не лежит в модуле, а передаётся
   в функции параметром. Индексы — map[y][x], строка, затем столбец.

   Почему передача массива не требует взятия адреса. В выражении имя
   массива почти всегда превращается в адрес его первого элемента.
   Первый элемент двумерного массива — это строка из MAP_COLS символов,
   поэтому параметр char map[][MAP_COLS] на самом деле указатель на
   строку: char (*map)[MAP_COLS]. Функция получает не копию трёхсот
   символов, а адрес, и меняет ту самую карту, что объявлена в game_run.
   Вторую размерность указывать обязательно: без неё компилятор не знает,
   на сколько байт сдвинуться, чтобы перейти к следующей строке. */

#define MAP_ROWS 10
#define MAP_COLS 30

#define TILE_WALL  '#'
#define TILE_FLOOR '.'
#define TILE_GOLD  '$'
#define TILE_EXIT  '>'

void map_init(char map[][MAP_COLS]);
int map_inside(int x, int y);
char map_at(char map[][MAP_COLS], int x, int y);
void map_set(char map[][MAP_COLS], int x, int y, char tile);
void map_draw(char map[][MAP_COLS], int hero_x, int hero_y);

/* Два результата сразу: что лежит в клетке и можно ли туда шагнуть.
   Оба возвращаются через выходные параметры, а обычным значением —
   признак успеха: 0, если клетка вне карты или передан нулевой указатель.
   Выходные параметры по обычаю стоят в конце списка. */
int map_cell_info(char map[][MAP_COLS], int x, int y, char *out_tile, int *out_walkable);

#endif /* MAP_H */
