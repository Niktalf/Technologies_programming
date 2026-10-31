#include <limits.h>
#include <stddef.h>
#include <stdlib.h>

#include "monsters.h"

#define MONSTERS_FIRST_CAPACITY 4

void monsters_init(Monsters *list) {
    if (list == NULL) {
        return;
    }
    list->cells = NULL;
    list->hps = NULL;
    list->count = 0;
    list->capacity = 0;
    list->growths = 0;
}

void monsters_destroy(Monsters *list) {
    if (list == NULL) {
        return;
    }
    free(list->cells);
    free(list->hps);
    monsters_init(list); /* указатели обнуляются, счётчики сбрасываются */
}

/* Удваивает ёмкость. Возвращает 1 при успехе. */
static int grow(Monsters *list) {
    if (list->capacity > INT_MAX / 2
        || (size_t)list->capacity * 2 > (size_t)-1 / sizeof(int)) {
        return 0; /* удвоение переполнило бы счётчик или размер блока */
    }
    int new_capacity = list->capacity == 0 ? MONSTERS_FIRST_CAPACITY : list->capacity * 2;

    /* Результат realloc — во временный указатель. Если выделить не удалось,
       realloc возвращает NULL, а старый блок остаётся действительным.
       Записав NULL прямо в list->cells, мы потеряли бы и старый блок. */
    int *bigger = realloc(list->cells, (size_t)new_capacity * sizeof(int));
    if (bigger == NULL) {
        return 0;
    }
    list->cells = bigger;

    bigger = realloc(list->hps, (size_t)new_capacity * sizeof(int));
    if (bigger == NULL) {
        /* Первый массив уже вырос, второй нет. Это не страшно: ёмкость
           не обновлена, и лишнее место в первом просто не используется. */
        return 0;
    }
    list->hps = bigger;
    list->capacity = new_capacity;
    ++list->growths;
    return 1;
}

int monsters_add(Monsters *list, int cell, int hp) {
    if (list == NULL) {
        return 0;
    }
    if (list->count == list->capacity && !grow(list)) {
        return 0;
    }
    list->cells[list->count] = cell;
    list->hps[list->count] = hp;
    ++list->count;
    return 1;
}

void monsters_remove(Monsters *list, int index) {
    if (list == NULL || index < 0 || index >= list->count) {
        return;
    }
    list->cells[index] = list->cells[list->count - 1];
    list->hps[index] = list->hps[list->count - 1];
    --list->count;
}

int monsters_find(const Monsters *list, int cell) {
    if (list == NULL) {
        return -1;
    }
    for (int i = 0; i < list->count; ++i) {
        if (list->cells[i] == cell) {
            return i;
        }
    }
    return -1;
}
