#ifndef STORE_H
#define STORE_H

#include <stddef.h>

/* Хранилище на практике 1: массивы фиксированного размера, поиск перебором.
   Хеш-таблица появится на практике 3, динамическая память — на практике 7,
   сохранение на диск — на практике 8. */

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
    int count; /* сколько пар занято: они лежат в элементах от 0 до count - 1 */
} Store;

void store_init(Store *store);

/* Кладёт значение по ключу. Существующий ключ обновляется, а не дублируется.
   Через was_present сообщает, был ли ключ до вызова; параметр можно не передавать. */
StoreStatus store_put(Store *store, const char *key, const char *value, int *was_present);

/* Отдаёт значение по ключу. Признак наличия — возвращаемое значение,
   само значение — через параметр: в хранилище может лежать любая строка. */
StoreStatus store_get(const Store *store, const char *key, const char **out_value);

StoreStatus store_remove(Store *store, const char *key);

int store_count(const Store *store);

/* Перебирает пары в порядке хранения. Индекс от нуля до store_count минус один. */
StoreStatus store_at(const Store *store, int index,
                     const char **out_key, const char **out_value);

const char *store_status_text(StoreStatus status);

#endif /* STORE_H */
