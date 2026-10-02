#ifndef STORE_H
#define STORE_H

#define STORE_CAPACITY  16384
#define STORE_KEY_MAX   32
#define STORE_VALUE_MAX 128

typedef enum {
    STORE_OK = 0,
    STORE_ERR_ARG,
    STORE_ERR_FULL,
    STORE_ERR_NOT_FOUND,
    STORE_ERR_TOO_LONG
} StoreStatus;

typedef enum {
    CELL_EMPTY = 0,
    CELL_USED,
    CELL_DELETED
} CellState;

typedef struct {
    CellState state;
    char      key[STORE_KEY_MAX];
    char      value[STORE_VALUE_MAX];
} Cell;

typedef struct {
    Cell cells[STORE_CAPACITY];
    int  count;
    int  deleted;
} Store;

typedef struct {
    int count;
    int capacity;
    int deleted;
    int displaced;
    int longest_probe;
} StoreStats;

void store_init(Store *store);

unsigned long store_hash(const char *key);

int store_home(const char *key);

StoreStatus store_put(Store *store, const char *key, const char *value, int *was_present);
StoreStatus store_get(const Store *store, const char *key, const char **out_value);
StoreStatus store_remove(Store *store, const char *key);
int         store_count(const Store *store);

StoreStatus store_get_linear(const Store *store, const char *key, const char **out_value);

void store_for_each(const Store *store,
                    void (*visit)(const char *key, const char *value, void *context),
                    void *context);

void store_stats(const Store *store, StoreStats *stats);

const char *store_status_text(StoreStatus status);

#endif // STORE_H
