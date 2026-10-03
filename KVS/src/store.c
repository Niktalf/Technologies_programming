#include <string.h>

#include "store.h"

#define INDEX_MASK (STORE_CAPACITY - 1)

void store_init(Store *store)
{
    if (store == NULL) {
        return;
    }
    for (int i = 0; i < STORE_CAPACITY; ++i) {
        store->cells[i].state = CELL_EMPTY;
    }
    store->count = 0;
    store->deleted = 0;
}

const char *store_status_text(const StoreStatus status)
{
    switch (status) {
        case STORE_OK:            return "success";
        case STORE_ERR_ARG:       return "invalid argument";
        case STORE_ERR_FULL:      return "storage is full";
        case STORE_ERR_NOT_FOUND: return "key not found";
        case STORE_ERR_TOO_LONG:  return "key or value too long";
        default:                  return "unknown error";
    }
}

unsigned long store_hash(const char *key)
{
    unsigned long hash = 5381;

    while (*key != '\0') {
        hash = hash * 33u + (unsigned char)*key;
        ++key;
    }
    return hash;
}

int store_home(const char *key)
{
    return (int)(store_hash(key) & INDEX_MASK);
}

static int find_cell(const Store *store, const char *key)
{
    int index = store_home(key);

    for (int probes = 0; probes < STORE_CAPACITY; ++probes) {
        const Cell *cell = &store->cells[index];

        if (cell->state == CELL_EMPTY) {
            return -1;
        }
        if (cell->state == CELL_USED && strcmp(cell->key, key) == 0) {
            return index;
        }
        index = (index + 1) & INDEX_MASK;
    }
    return -1;
}

StoreStatus store_put(Store *store, const char *key, const char *value, int *was_present)
{

    if (store == NULL || key == NULL || value == NULL || key[0] == '\0') {
        return STORE_ERR_ARG;
    }
    if (strlen(key) >= STORE_KEY_MAX || strlen(value) >= STORE_VALUE_MAX) {
        return STORE_ERR_TOO_LONG;
    }

    int probes;
    int first_deleted = -1;

    int index = store_home(key);
    for (probes = 0; probes < STORE_CAPACITY; ++probes) {
        Cell *cell = &store->cells[index];

        if (cell->state == CELL_EMPTY) {
            break;
        }
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
        index = (index + 1) & INDEX_MASK;
    }

    if (first_deleted >= 0) {
        index = first_deleted;
        --store->deleted;
    } else if (probes == STORE_CAPACITY) {
        return STORE_ERR_FULL;
    }

    store->cells[index].state = CELL_USED;
    strcpy(store->cells[index].key, key);
    strcpy(store->cells[index].value, value);
    ++store->count;
    if (was_present != NULL) {
        *was_present = 0;
    }
    return STORE_OK;
}

StoreStatus store_get(const Store *store, const char *key, const char **out_value)
{
    if (store == NULL || key == NULL || out_value == NULL) {
        return STORE_ERR_ARG;
    }
    const int index = find_cell(store, key);
    if (index < 0) {
        return STORE_ERR_NOT_FOUND;
    }
    *out_value = store->cells[index].value;
    return STORE_OK;
}

StoreStatus store_get_linear(const Store *store, const char *key, const char **out_value)
{
    int i;

    if (store == NULL || key == NULL || out_value == NULL) {
        return STORE_ERR_ARG;
    }
    for (i = 0; i < STORE_CAPACITY; ++i) {
        if (store->cells[i].state == CELL_USED && strcmp(store->cells[i].key, key) == 0) {
            *out_value = store->cells[i].value;
            return STORE_OK;
        }
    }
    return STORE_ERR_NOT_FOUND;
}

StoreStatus store_remove(Store *store, const char *key)
{
    if (store == NULL || key == NULL) {
        return STORE_ERR_ARG;
    }
    const int index = find_cell(store, key);
    if (index < 0) {
        return STORE_ERR_NOT_FOUND;
    }

    store->cells[index].state = CELL_DELETED;
    --store->count;
    ++store->deleted;
    return STORE_OK;
}

int store_count(const Store *store)
{
    return store != NULL ? store->count : 0;
}

void store_for_each(const Store *store,
                    void (*visit)(const char *key, const char *value, void *context),
                    void *context)
{
    if (store == NULL || visit == NULL) {
        return;
    }

    for (int i = 0; i < STORE_CAPACITY; ++i) {
        if (store->cells[i].state == CELL_USED) {
            visit(store->cells[i].key, store->cells[i].value, context);
        }
    }
}

void store_stats(const Store *store, StoreStats *stats)
{
    stats->count = store->count;
    stats->capacity = STORE_CAPACITY;
    stats->deleted = store->deleted;
    stats->displaced = 0;
    stats->longest_probe = 0;

    for (int i = 0; i < STORE_CAPACITY; ++i) {
        const Cell *cell = &store->cells[i];

        if (cell->state == CELL_USED) {
            const int distance = (i - store_home(cell->key)) & INDEX_MASK;

            if (distance > 0) {
                ++stats->displaced;
            }
            if (distance + 1 > stats->longest_probe) {
                stats->longest_probe = distance + 1;
            }
        }
    }
}
