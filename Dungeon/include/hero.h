#ifndef HERO_H
#define HERO_H

#include "map.h"

/* Практика 2: состояния героя как набор битов в одной беззнаковой
   переменной. Маска — единица, сдвинутая на номер бита. */

#define STATE_POISONED  (1u << 0) /* отравлен */
#define STATE_BLINDED   (1u << 1) /* ослеплён */
#define STATE_INVISIBLE (1u << 2) /* невидим */
#define STATE_PETRIFIED (1u << 3) /* окаменел */
#define STATE_BURNING   (1u << 4) /* горит */
#define STATE_ASLEEP    (1u << 5) /* спит */

#define STATE_COUNT 6

#define DIRECTION_NORTH 0
#define DIRECTION_SOUTH 1
#define DIRECTION_EAST  2
#define DIRECTION_WEST  3

#define GOLD_PER_PILE 10

/* Практика 5: функции получают адреса переменных героя и меняют их сами,
   а не возвращают новые значения. Переменные героя теперь локальные
   в game_run, глобальных больше нет: по списку параметров видно,
   что именно функция может изменить. */

/* Сдвигает героя на клетку в направлении direction, если там не стена.
   Возвращает 1, если шаг сделан. Координаты меняет по адресам x и y. */
int hero_move(char map[][MAP_COLS], int *x, int *y, int direction);

/* Отнимает amount здоровья; ниже нуля здоровье не опускается. */
void hero_damage(int *hp, int amount);

/* Если герой стоит на золоте, прибавляет его к *gold и убирает с карты.
   Возвращает 1, если золото было. */
int hero_pick_gold(char map[][MAP_COLS], int x, int y, unsigned int *gold);

/* Практика 6: предметы в сумке героя. Сумка — массив номеров предметов
   фиксированной вместимости и счётчик заполненных мест. */
#define ITEM_POTION   1 /* зелье: +10 здоровья */
#define ITEM_ANTIDOTE 2 /* противоядие: снимает отравление */
#define ITEM_TORCH    3 /* факел: снимает слепоту */

#define INVENTORY_SIZE 8
#define POTION_HEAL    10

const char *hero_item_name(int item);

/* Применяет предмет с номером number в сумке (нумерация с единицы,
   как видит игрок) и убирает его из сумки. Возвращает 1, если предмет
   с таким номером был. */
int hero_use_item(int items[], int *item_count, int number,
                  int *hp, int hp_max, unsigned int *states);

#endif /* HERO_H */
