#include <string.h>

#include "store.h"

#define INDEX_MASK (STORE_CAPACITY - 1)

void store_init(Store *store) {
    if (store == NULL) {
        return;
    }
    /* Ключи и значения не очищаются: у пустой ячейки они не читаются. */
    for (int i = 0; i < STORE_CAPACITY; ++i) {
        store->cells[i].state = CELL_EMPTY;
    }
    store->count = 0;
    store->deleted = 0;
}

const char *store_status_text(StoreStatus status) {
    switch (status) {
    case STORE_OK:
        return "success";
    case STORE_ERR_ARG:
        return "invalid argument";
    case STORE_ERR_FULL:
        return "store is full";
    case STORE_ERR_NOT_FOUND:
        return "key not found";
    case STORE_ERR_TOO_LONG:
        return "key or value too long";
    default:
        return "unknown error";
    }
}

unsigned long store_hash(const char *key) {
    /* Схема djb2: умножение на 33 и прибавление очередного символа.
       Тип беззнаковый: переполнение для него определено, а остаток
       от деления отрицательного числа дал бы отрицательный номер ячейки. */
    unsigned long hash = 5381;
    for (const char *p = key; *p != '\0'; ++p) {
        hash = hash * 33u + (unsigned char)*p;
    }
    return hash;
}

int store_home(const char *key) {
    /* Ёмкость — степень двойки, поэтому остаток от деления
       заменяется побитовым И с маской. */
    return (int)(store_hash(key) & INDEX_MASK);
}

/* Следующая ячейка цепочки проб; после последней — нулевая. */
static int next_index(int index) {
    return (index + 1) & INDEX_MASK;
}

/* Ищет ячейку с ключом. Возвращает её номер или -1.
   Проб не больше ёмкости: на заполненной таблице без этого ограничения
   поиск отсутствующего ключа не закончился бы никогда. */
static int find_cell(const Store *store, const char *key) {
    int index = store_home(key);
    for (int probes = 0; probes < STORE_CAPACITY; ++probes) {
        const Cell *cell = &store->cells[index];
        if (cell->state == CELL_EMPTY) {
            return -1; /* цепочку обрывает только пустая ячейка */
        }
        if (cell->state == CELL_USED && strcmp(cell->key, key) == 0) {
            return index;
        }
        /* Удалённую ячейку проходим дальше: за ней могут быть наши ключи. */
        index = next_index(index);
    }
    return -1;
}

StoreStatus store_put(Store *store, const char *key, const char *value, int *was_present) {
    if (store == NULL || key == NULL || value == NULL || key[0] == '\0') {
        return STORE_ERR_ARG;
    }
    if (strlen(key) >= STORE_KEY_MAX || strlen(value) >= STORE_VALUE_MAX) {
        return STORE_ERR_TOO_LONG;
    }

    /* Идём по цепочке проб до пустой ячейки: ищем ключ и заодно запоминаем
       первую удалённую ячейку — если ключа нет, займём её, а не пустую дальше. */
    int index = store_home(key);
    int first_deleted = -1;
    int probes = 0;
    while (probes < STORE_CAPACITY && store->cells[index].state != CELL_EMPTY) {
        Cell *cell = &store->cells[index];
        if (cell->state == CELL_DELETED) {
            if (first_deleted < 0) {
                first_deleted = index;
            }
        } else if (strcmp(cell->key, key) == 0) {
            strcpy(cell->value, value);
            if (was_present != NULL) {
                *was_present = 1;
            }
            return STORE_OK;
        }
        index = next_index(index);
        ++probes;
    }

    if (first_deleted >= 0) {
        index = first_deleted;
        --store->deleted;
    } else if (probes == STORE_CAPACITY) {
        return STORE_ERR_FULL; /* обошли всю таблицу, свободных ячеек нет */
    }

    Cell *cell = &store->cells[index];
    cell->state = CELL_USED;
    strcpy(cell->key, key);
    strcpy(cell->value, value);
    ++store->count;
    if (was_present != NULL) {
        *was_present = 0;
    }
    return STORE_OK;
}

StoreStatus store_get(const Store *store, const char *key, const char **out_value) {
    if (store == NULL || key == NULL || out_value == NULL) {
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
    if (store == NULL || key == NULL || out_value == NULL) {
        return STORE_ERR_ARG;
    }
    for (int i = 0; i < STORE_CAPACITY; ++i) {
        const Cell *cell = &store->cells[i];
        if (cell->state == CELL_USED && strcmp(cell->key, key) == 0) {
            *out_value = cell->value;
            return STORE_OK;
        }
    }
    return STORE_ERR_NOT_FOUND;
}

StoreStatus store_remove(Store *store, const char *key) {
    if (store == NULL || key == NULL) {
        return STORE_ERR_ARG;
    }
    int index = find_cell(store, key);
    if (index < 0) {
        return STORE_ERR_NOT_FOUND;
    }
    /* Не «пусто», а «удалено»: иначе цепочка проб оборвётся. */
    store->cells[index].state = CELL_DELETED;
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
    if (store == NULL || visit == NULL) {
        return;
    }
    for (int i = 0; i < STORE_CAPACITY; ++i) {
        const Cell *cell = &store->cells[i];
        if (cell->state == CELL_USED) {
            visit(cell->key, cell->value, context);
        }
    }
}

void store_stats(const Store *store, StoreStats *stats) {
    stats->count = store->count;
    stats->capacity = STORE_CAPACITY;
    stats->deleted = store->deleted;
    stats->displaced = 0;
    stats->longest_probe = 0;

    for (int i = 0; i < STORE_CAPACITY; ++i) {
        const Cell *cell = &store->cells[i];
        if (cell->state == CELL_USED) {
            /* Сколько шагов от домашней ячейки до фактической, с учётом
               перехода через конец таблицы. Маска делает разность неотрицательной. */
            int distance = (i - store_home(cell->key)) & INDEX_MASK;
            if (distance > 0) {
                ++stats->displaced;
            }
            if (distance + 1 > stats->longest_probe) {
                stats->longest_probe = distance + 1;
            }
        }
    }
}
