#ifndef STORE_H
#define STORE_H

#define STORE_CAPACITY  64
#define STORE_KEY_MAX   32
#define STORE_VALUE_MAX 128

typedef enum {
    STORE_OK = 0,
    STORE_ERR_ARG,
    STORE_ERR_FULL,
    STORE_ERR_NOT_FOUND,
    STORE_ERR_TOO_LONG
} StoreStatus;

typedef struct {
    char keys[STORE_CAPACITY][STORE_KEY_MAX];
    char values[STORE_CAPACITY][STORE_VALUE_MAX];
    int  count;
} Store;

void store_init(Store *store);

StoreStatus store_put(Store *store, const char *key, const char *value, int *was_present);

StoreStatus store_get(const Store *store, const char *key, const char **out_value);

StoreStatus store_remove(Store *store, const char *key);

int store_count(const Store *store);

StoreStatus store_at(const Store *store, int index, const char **out_key, const char **out_value);

const char *store_status_text(StoreStatus status);

#endif // STORE_H
