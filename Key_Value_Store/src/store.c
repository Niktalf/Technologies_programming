#include <stdlib.h>
#include <string.h>

#include "store.h"

const char *store_status_text(StoreStatus status) {
    switch (status) {
    case STORE_OK:
        return "success";
    case STORE_ERR_ARG:
        return "invalid argument";
    case STORE_ERR_MEMORY:
        return "not enough memory";
    case STORE_ERR_NOT_FOUND:
        return "key not found";
    default:
        return "unknown error";
    }
}

unsigned long store_hash(const char *key) {
    /* Схема djb2: умножение на 33 и прибавление очередного символа.
       Тип беззнаковый: переполнение для него определено. */
    unsigned long hash = 5381;
    for (const char *p = key; *p != '\0'; ++p) {
        hash = hash * 33u + (unsigned char)*p;
    }
    return hash;
}

/* Домашняя ячейка ключа в таблице вместимости capacity. Вместимость —
   степень двойки, поэтому остаток от деления заменяется побитовым И. */
static int home_in(int capacity, const char *key) {
    return (int)(store_hash(key) & (unsigned long)(capacity - 1));
}

int store_home(const Store *store, const char *key) {
    return home_in(store->capacity, key);
}

int store_init(Store *store) {
    if (store == NULL) {
        return 0;
    }
    store->capacity = 0;
    store->count = 0;
    store->deleted = 0;
    store->rehash_count = 0;
    /* calloc обнуляет память: все ячейки сразу пустые, указатели NULL. */
    store->cells = calloc(STORE_FIRST_CAPACITY, sizeof(Cell));
    if (store->cells == NULL) {
        return 0;
    }
    store->capacity = STORE_FIRST_CAPACITY;
    return 1;
}

void store_destroy(Store *store) {
    if (store == NULL || store->cells == NULL) {
        return;
    }
    /* Сначала строки каждой ячейки, потом сам массив ячеек: в обратном
       порядке адреса строк были бы уже потеряны. У пустых ячеек указатели
       NULL, и free(NULL) ничего не делает. */
    for (int i = 0; i < store->capacity; ++i) {
        free(store->cells[i].key);
        free(store->cells[i].value);
    }
    free(store->cells);
    store->cells = NULL;
    store->capacity = 0;
    store->count = 0;
    store->deleted = 0;
}

/* Кладёт готовые строки в таблицу, ничего не выделяя: строки переходят
   во владение таблицы. Используется и вставкой, и перестроением.
   Свободная ячейка есть всегда: таблица растёт заранее. */
static void place(Cell *cells, int capacity, char *key, char *value) {
    int index = home_in(capacity, key);
    while (cells[index].state == CELL_USED) {
        index = (index + 1) & (capacity - 1);
    }
    cells[index].state = CELL_USED;
    cells[index].key = key;
    cells[index].value = value;
}

/* Создаёт вдвое большую таблицу и переносит занятые ячейки. Номер ячейки
   зависит от вместимости, поэтому ключи раскладываются заново. */
static int rehash(Store *store) {
    int new_capacity = store->capacity * 2;
    Cell *cells = calloc((size_t)new_capacity, sizeof(Cell));
    if (cells == NULL) {
        return 0; /* старая таблица цела, работа продолжается */
    }
    for (int i = 0; i < store->capacity; ++i) {
        Cell *old = &store->cells[i];
        if (old->state == CELL_USED) {
            /* Строки переносятся по указателю: копировать их заново не нужно. */
            place(cells, new_capacity, old->key, old->value);
        }
    }
    /* У удалённых ячеек строки уже освобождены в store_remove,
       поэтому освобождается только старый массив ячеек. */
    free(store->cells);
    store->cells = cells;
    store->capacity = new_capacity;
    store->deleted = 0;
    ++store->rehash_count;
    return 1;
}

static int find_cell(const Store *store, const char *key) {
    int index = store_home(store, key);
    for (int probes = 0; probes < store->capacity; ++probes) {
        const Cell *cell = &store->cells[index];
        if (cell->state == CELL_EMPTY) {
            return -1; /* цепочку обрывает только пустая ячейка */
        }
        if (cell->state == CELL_USED && strcmp(cell->key, key) == 0) {
            return index;
        }
        /* Удалённую ячейку проходим дальше: за ней могут быть наши ключи. */
        index = (index + 1) & (store->capacity - 1);
    }
    return -1;
}

/* Копия строки в куче по её длине или NULL, если памяти не хватило. */
static char *duplicate(const char *text) {
    size_t length = strlen(text) + 1;
    char *copy = malloc(length);
    if (copy != NULL) {
        memcpy(copy, text, length);
    }
    return copy;
}

/* Обновляет значение существующего ключа. Новое значение готовится до того,
   как выбросить старое: при нехватке памяти прежнее значение уцелеет. */
static StoreStatus replace_value(Cell *cell, const char *value) {
    char *value_copy = duplicate(value);
    if (value_copy == NULL) {
        return STORE_ERR_MEMORY;
    }
    free(cell->value);
    cell->value = value_copy;
    return STORE_OK;
}

StoreStatus store_put(Store *store, const char *key, const char *value, int *was_present) {
    if (store == NULL || store->cells == NULL || key == NULL || value == NULL
        || key[0] == '\0') {
        return STORE_ERR_ARG;
    }
    int index = find_cell(store, key);
    if (was_present != NULL) {
        *was_present = index >= 0;
    }
    if (index >= 0) {
        return replace_value(&store->cells[index], value);
    }

    /* Растём по заполненности, а не когда места уже нет: в полной таблице
       поиск выродился бы в перебор. Удалённые ячейки тоже удлиняют цепочки,
       поэтому считаются вместе с занятыми. */
    if ((store->count + store->deleted + 1) * 100 >= store->capacity * STORE_LOAD_PERCENT
        && !rehash(store)) {
        return STORE_ERR_MEMORY;
    }

    char *key_copy = duplicate(key);
    char *value_copy = duplicate(value);
    if (key_copy == NULL || value_copy == NULL) {
        /* Одна из копий могла выделиться: free(NULL) безопасен. */
        free(key_copy);
        free(value_copy);
        return STORE_ERR_MEMORY;
    }
    place(store->cells, store->capacity, key_copy, value_copy);
    ++store->count;
    return STORE_OK;
}

StoreStatus store_get(const Store *store, const char *key, const char **out_value) {
    if (store == NULL || store->cells == NULL || key == NULL || out_value == NULL) {
        return STORE_ERR_ARG;
    }
    int index = find_cell(store, key);
    if (index < 0) {
        return STORE_ERR_NOT_FOUND;
    }
    *out_value = store->cells[index].value;
    return STORE_OK;
}

StoreStatus store_get_linear(const Store *store, const char *key, const char **out_value) {
    if (store == NULL || store->cells == NULL || key == NULL || out_value == NULL) {
        return STORE_ERR_ARG;
    }
    for (int i = 0; i < store->capacity; ++i) {
        const Cell *cell = &store->cells[i];
        if (cell->state == CELL_USED && strcmp(cell->key, key) == 0) {
            *out_value = cell->value;
            return STORE_OK;
        }
    }
    return STORE_ERR_NOT_FOUND;
}

StoreStatus store_remove(Store *store, const char *key) {
    if (store == NULL || store->cells == NULL || key == NULL) {
        return STORE_ERR_ARG;
    }
    int index = find_cell(store, key);
    if (index < 0) {
        return STORE_ERR_NOT_FOUND;
    }
    /* Строки освобождаем сразу, ячейку помечаем удалённой, а не пустой:
       иначе цепочка проб оборвётся и ключи за ней потеряются. */
    Cell *cell = &store->cells[index];
    free(cell->key);
    free(cell->value);
    cell->key = NULL;
    cell->value = NULL;
    cell->state = CELL_DELETED;
    --store->count;
    ++store->deleted;
    return STORE_OK;
}

int store_count(const Store *store) {
    return store != NULL ? store->count : 0;
}

void store_for_each(const Store *store,
                    void (*visit)(const char *key, const char *value, void *context),
                    void *context) {
    if (store == NULL || store->cells == NULL || visit == NULL) {
        return;
    }
    for (int i = 0; i < store->capacity; ++i) {
        const Cell *cell = &store->cells[i];
        if (cell->state == CELL_USED) {
            visit(cell->key, cell->value, context);
        }
    }
}

void store_stats(const Store *store, StoreStats *stats) {
    stats->count = store->count;
    stats->capacity = store->capacity;
    stats->deleted = store->deleted;
    stats->displaced = 0;
    stats->longest_probe = 0;
    stats->rehash_count = store->rehash_count;

    for (int i = 0; i < store->capacity; ++i) {
        const Cell *cell = &store->cells[i];
        if (cell->state == CELL_USED) {
            /* Сколько шагов от домашней ячейки до фактической, с учётом
               перехода через конец таблицы. Маска делает разность неотрицательной. */
            int distance = (i - store_home(store, cell->key)) & (store->capacity - 1);
            if (distance > 0) {
                ++stats->displaced;
            }
            if (distance + 1 > stats->longest_probe) {
                stats->longest_probe = distance + 1;
            }
        }
    }
}
