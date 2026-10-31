#ifndef ENTITIES_H
#define ENTITIES_H

#include "map.h"
#include "status.h"

/* Практика 8: сущности подземелья — герой и монстры — описаны структурами.
   Вместо десятка отдельных переменных функции получают адрес одной
   структуры. */

/* Состояния героя: по биту на состояние (практика 2). */
#define STATE_POISONED  (1u << 0)
#define STATE_BLINDED   (1u << 1)
#define STATE_INVISIBLE (1u << 2)
#define STATE_PETRIFIED (1u << 3)
#define STATE_BURNING   (1u << 4)
#define STATE_ASLEEP    (1u << 5)
#define STATE_COUNT     6

#define DIRECTION_NORTH 0
#define DIRECTION_SOUTH 1
#define DIRECTION_EAST  2
#define DIRECTION_WEST  3

#define GOLD_PER_PILE  10
#define MONSTER_HP     8

/* Предметы (практика 6). */
#define ITEM_POTION    1
#define ITEM_ANTIDOTE  2
#define ITEM_TORCH     3
#define INVENTORY_SIZE 8
#define POTION_HEAL    10

/* Почему выбраны такие типы — см. практику 2. */
typedef struct {
    int x;
    int y;
    int hp;               /* знаковое: урон может увести ниже нуля до проверки */
    int hp_max;
    unsigned char attack; /* небольшое неотрицательное число */
    unsigned int gold;    /* только копится */
    int depth;            /* номер этажа */
    unsigned int states;  /* набор битов STATE_* */
    int items[INVENTORY_SIZE];
    int item_count;
} Hero;

typedef struct {
    int x;
    int y;
    int hp;
} Monster;

/* Растущий список монстров (практика 7). Теперь это один массив структур,
   а не два параллельных массива. */
typedef struct {
    Monster *items; /* массив в куче */
    int count;      /* сколько монстров сейчас */
    int capacity;   /* на сколько хватает выделенной памяти */
    int growths;    /* сколько раз ёмкость удваивалась — для наглядности */
} MonsterList;

/* --- Герой --- */

/* Начальные значения: полное здоровье, четыре предмета в сумке. */
void hero_init(Hero *hero, int x, int y);

/* Куда ведёт шаг: две координаты через выходные параметры.
   Возвращает 0 при неверном направлении. */
int hero_target(const Hero *hero, int direction, int *out_x, int *out_y);

/* Шаг, если там не стена. Возвращает 1, если шаг сделан. */
int hero_move(Hero *hero, const Map *map, int direction);

void hero_damage(Hero *hero, int amount);

/* Подбирает золото под героем. Возвращает 1, если золото было. */
int hero_pick_gold(Hero *hero, Map *map);

const char *hero_item_name(int item);

/* Применяет предмет номер number (с единицы). Возвращает 1, если он был. */
int hero_use_item(Hero *hero, int number);

/* --- Монстры --- */

void monsters_init(MonsterList *list);

/* Освобождает массив и обнуляет указатель. Повторный вызов безопасен. */
void monsters_destroy(MonsterList *list);

/* STATUS_NO_MEMORY, если не удалось вырастить список; тогда он прежний. */
Status monsters_add(MonsterList *list, int x, int y, int hp);

/* Удаляет монстра номер index: на его место переносится последний. */
void monsters_remove(MonsterList *list, int index);

/* Номер монстра в клетке (x, y) или -1. */
int monsters_find(const MonsterList *list, int x, int y);

#endif /* ENTITIES_H */
