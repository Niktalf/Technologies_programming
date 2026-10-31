#include <stddef.h>

#include "hero.h"

int hero_target(int x, int y, int direction, int *out_x, int *out_y) {
    static const int DX[4] = { 0, 0, 1, -1 };
    static const int DY[4] = { -1, 1, 0, 0 };

    if (out_x == NULL || out_y == NULL || direction < 0 || direction > 3) {
        return 0;
    }
    *out_x = x + DX[direction];
    *out_y = y + DY[direction];
    return 1;
}

int hero_move(const Map *map, int *x, int *y, int direction) {
    if (map == NULL || x == NULL || y == NULL) {
        return 0;
    }
    int nx = 0;
    int ny = 0;
    if (!hero_target(*x, *y, direction, &nx, &ny)) {
        return 0;
    }
    /* Клетка вне карты или стена — шага нет. map_cell_info отвечает
       на оба вопроса сразу и сама проверяет границы. */
    char tile = TILE_WALL;
    int walkable = 0;
    if (!map_cell_info(map, nx, ny, &tile, &walkable) || !walkable) {
        return 0;
    }
    /* Пишем по адресу: меняется переменная вызывающего, а не копия. */
    *x = nx;
    *y = ny;
    return 1;
}

void hero_damage(int *hp, int amount) {
    if (hp == NULL) {
        return;
    }
    /* Скобки не нужны, но полезно помнить: *hp - amount — это значение
       по адресу минус урон, а *hp++ сдвинул бы сам указатель. */
    *hp = *hp - amount;
    if (*hp < 0) {
        *hp = 0;
    }
}

int hero_pick_gold(Map *map, int x, int y, unsigned int *gold) {
    if (map == NULL || gold == NULL || map_at(map, x, y) != TILE_GOLD) {
        return 0;
    }
    *gold = *gold + GOLD_PER_PILE;
    map_set(map, x, y, TILE_FLOOR);
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

int hero_use_item(int items[], int *item_count, int number,
                  int *hp, int hp_max, unsigned int *states) {
    if (items == NULL || item_count == NULL || hp == NULL || states == NULL) {
        return 0;
    }
    if (number < 1 || number > *item_count) {
        return 0;
    }
    int index = number - 1;
    int item = items[index];
    if (item == ITEM_POTION) {
        *hp = *hp + POTION_HEAL;
        if (*hp > hp_max) {
            *hp = hp_max;
        }
    } else if (item == ITEM_ANTIDOTE) {
        *states = *states & ~STATE_POISONED;
    } else if (item == ITEM_TORCH) {
        *states = *states & ~STATE_BLINDED;
    }

    /* Предмет израсходован: хвост сумки сдвигается на его место,
       чтобы порядок остальных предметов не изменился. */
    for (int i = index; i < *item_count - 1; ++i) {
        items[i] = items[i + 1];
    }
    --*item_count;
    return 1;
}
