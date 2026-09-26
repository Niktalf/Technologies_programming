#include <string.h>

#include "store.h"

void store_init(Store *store)
{
    if (store == NULL) {
        return;
    }
    store->count = 0;
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

static int find_index(const Store *store, const char *key)
{
    int i;

    for (i = 0; i < store->count; ++i) {
        if (strcmp(store->keys[i], key) == 0) {
            return i;
        }
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

    const int index = find_index(store, key);
    if (index >= 0) {
        strcpy(store->values[index], value);
        if (was_present != NULL) {
            *was_present = 1;
        }
        return STORE_OK;
    }

    if (store->count >= STORE_CAPACITY) {
        return STORE_ERR_FULL;
    }
    strcpy(store->keys[store->count], key);
    strcpy(store->values[store->count], value);
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

    const int index = find_index(store, key);
    if (index < 0) {
        return STORE_ERR_NOT_FOUND;
    }
    *out_value = store->values[index];
    return STORE_OK;
}

StoreStatus store_remove(Store *store, const char *key)
{
    if (store == NULL || key == NULL) {
        return STORE_ERR_ARG;
    }
    const int index = find_index(store, key);
    if (index < 0) {
        return STORE_ERR_NOT_FOUND;
    }

    --store->count;
    if (index != store->count) {
        strcpy(store->keys[index], store->keys[store->count]);
        strcpy(store->values[index], store->values[store->count]);
    }
    return STORE_OK;
}

int store_count(const Store *store)
{
    return store != NULL ? store->count : 0;
}

StoreStatus store_at(const Store *store, const int index, const char **out_key, const char **out_value)
{
    if (store == NULL || out_key == NULL || out_value == NULL) {
        return STORE_ERR_ARG;
    }
    if (index < 0 || index >= store->count) {
        return STORE_ERR_NOT_FOUND;
    }
    *out_key = store->keys[index];
    *out_value = store->values[index];
    return STORE_OK;
}
