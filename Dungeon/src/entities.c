#include <limits.h>
#include <stddef.h>
#include <stdlib.h>

#include "entities.h"

#define MONSTERS_FIRST_CAPACITY 4

void hero_init(Hero *hero, int x, int y) {
    if (hero == NULL) {
        return;
    }
    hero->x = x;
    hero->y = y;
    hero->hp = 30;
    hero->hp_max = 30;
    hero->attack = 5;
    hero->gold = 0;
    hero->depth = 1;
    hero->states = 0;
    hero->items[0] = ITEM_POTION;
    hero->items[1] = ITEM_ANTIDOTE;
    hero->items[2] = ITEM_TORCH;
    hero->items[3] = ITEM_POTION;
    hero->item_count = 4;
}

int hero_target(const Hero *hero, int direction, int *out_x, int *out_y) {
    static const int DX[4] = { 0, 0, 1, -1 };
    static const int DY[4] = { -1, 1, 0, 0 };

    if (hero == NULL || out_x == NULL || out_y == NULL || direction < 0 || direction > 3) {
        return 0;
    }
    *out_x = hero->x + DX[direction];
    *out_y = hero->y + DY[direction];
    return 1;
}

int hero_move(Hero *hero, const Map *map, int direction) {
    int nx = 0;
    int ny = 0;
    if (hero == NULL || map == NULL || !hero_target(hero, direction, &nx, &ny)) {
        return 0;
    }
    /* Клетка вне карты или стена — шага нет. map_cell_info отвечает
       на оба вопроса сразу и сама проверяет границы. */
    char tile = TILE_WALL;
    int walkable = 0;
    if (!map_cell_info(map, nx, ny, &tile, &walkable) || !walkable) {
        return 0;
    }
    hero->x = nx;
    hero->y = ny;
    return 1;
}

void hero_damage(Hero *hero, int amount) {
    if (hero == NULL) {
        return;
    }
    hero->hp = hero->hp - amount;
    if (hero->hp < 0) {
        hero->hp = 0;
    }
}

int hero_pick_gold(Hero *hero, Map *map) {
    if (hero == NULL || map_at(map, hero->x, hero->y) != TILE_GOLD) {
        return 0;
    }
    hero->gold = hero->gold + GOLD_PER_PILE;
    map_set(map, hero->x, hero->y, TILE_FLOOR);
    return 1;
}

const char *hero_item_name(int item) {
    switch (item) {
    case ITEM_POTION:
        return "healing potion";
    case ITEM_ANTIDOTE:
        return "antidote";
    case ITEM_TORCH:
        return "torch";
    default:
        return "unknown item";
    }
}

int hero_use_item(Hero *hero, int number) {
    if (hero == NULL || number < 1 || number > hero->item_count) {
        return 0;
    }
    int index = number - 1;
    int item = hero->items[index];
    if (item == ITEM_POTION) {
        hero->hp = hero->hp + POTION_HEAL;
        if (hero->hp > hero->hp_max) {
            hero->hp = hero->hp_max;
        }
    } else if (item == ITEM_ANTIDOTE) {
        hero->states = hero->states & ~STATE_POISONED;
    } else if (item == ITEM_TORCH) {
        hero->states = hero->states & ~STATE_BLINDED;
    }
    /* Предмет израсходован: хвост сумки сдвигается на его место,
       чтобы порядок остальных предметов не изменился. */
    for (int i = index; i < hero->item_count - 1; ++i) {
        hero->items[i] = hero->items[i + 1];
    }
    --hero->item_count;
    return 1;
}

void monsters_init(MonsterList *list) {
    if (list == NULL) {
        return;
    }
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
    list->growths = 0;
}

void monsters_destroy(MonsterList *list) {
    if (list == NULL) {
        return;
    }
    free(list->items);
    monsters_init(list); /* указатель обнуляется, повторный вызов безопасен */
}

/* Удваивает ёмкость списка. Один массив структур — один realloc,
   и разойтись нечему: на практике 7 два параллельных массива росли
   каждый своим realloc. */
static Status grow(MonsterList *list) {
    if (list->capacity > INT_MAX / 2
        || (size_t)list->capacity * 2 > (size_t)-1 / sizeof(Monster)) {
        return STATUS_NO_MEMORY; /* удвоение переполнило бы счётчик или размер блока */
    }
    int new_capacity = list->capacity == 0 ? MONSTERS_FIRST_CAPACITY : list->capacity * 2;
    /* Результат realloc — во временный указатель: при неудаче старый блок цел. */
    Monster *bigger = realloc(list->items, (size_t)new_capacity * sizeof(Monster));
    if (bigger == NULL) {
        return STATUS_NO_MEMORY;
    }
    list->items = bigger;
    list->capacity = new_capacity;
    ++list->growths;
    return STATUS_OK;
}

Status monsters_add(MonsterList *list, int x, int y, int hp) {
    if (list == NULL) {
        return STATUS_NO_MEMORY;
    }
    if (list->count == list->capacity) {
        Status status = grow(list);
        if (status != STATUS_OK) {
            return status; /* список прежний */
        }
    }
    Monster monster = { x, y, hp };
    list->items[list->count] = monster; /* структура копируется одним присваиванием */
    ++list->count;
    return STATUS_OK;
}

void monsters_remove(MonsterList *list, int index) {
    if (list == NULL || index < 0 || index >= list->count) {
        return;
    }
    /* Структура копируется целиком одним присваиванием. */
    list->items[index] = list->items[list->count - 1];
    --list->count;
}

int monsters_find(const MonsterList *list, int x, int y) {
    if (list == NULL) {
        return -1;
    }
    for (int i = 0; i < list->count; ++i) {
        if (list->items[i].x == x && list->items[i].y == y) {
            return i;
        }
    }
    return -1;
}
