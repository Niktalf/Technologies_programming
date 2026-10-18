#ifndef STORE_H
#define STORE_H

#include <stddef.h>

/* Практика 3: хеш-таблица с открытой адресацией и линейным пробированием.
   Интерфейс модуля тот же, что на практике 1: команды не знают,
   как хранилище устроено внутри, и переписывать их не пришлось.
   Размер таблицы фиксирован; рост таблицы появится на практике 7. */

#define STORE_CAPACITY  16384 /* степень двойки: номер ячейки берётся маской */
#define STORE_KEY_MAX   32
#define STORE_VALUE_MAX 128

typedef enum {
    STORE_OK = 0,
    STORE_ERR_ARG,
    STORE_ERR_FULL,
    STORE_ERR_NOT_FOUND,
    STORE_ERR_TOO_LONG
} StoreStatus;

/* Три состояния ячейки. Третье, «удалено», обязательно: если пометить
   удалённую ячейку пустой, поиск остановится на ней и не найдёт ключи,
   которые при вставке прошли через неё дальше по цепочке проб. */
typedef enum {
    CELL_EMPTY = 0,
    CELL_USED,
    CELL_DELETED
} CellState;

typedef struct {
    CellState state;
    char key[STORE_KEY_MAX];
    char value[STORE_VALUE_MAX];
} Cell;

typedef struct {
    Cell cells[STORE_CAPACITY];
    int count;   /* занятых ячеек */
    int deleted; /* ячеек с пометкой «удалено» */
} Store;

/* Объект хранилища весит около 2,7 МБ и поэтому объявляется статическим. */

typedef struct {
    int count;
    int capacity;
    int deleted;
    int displaced;     /* ключей, лежащих не в своей домашней ячейке */
    int longest_probe; /* самая длинная цепочка проб до ключа */
} StoreStats;

void store_init(Store *store);

/* Хеш строки. Сумма кодов символов не годится: у слов из одних
   и тех же букв она одинакова. */
unsigned long store_hash(const char *key);

/* Домашняя ячейка ключа — та, с которой начинается поиск. */
int store_home(const char *key);

StoreStatus store_put(Store *store, const char *key, const char *value, int *was_present);
StoreStatus store_get(const Store *store, const char *key, const char **out_value);
StoreStatus store_remove(Store *store, const char *key);
int store_count(const Store *store);

/* Поиск перебором всех ячеек — для сравнения скорости с хешированием. */
StoreStatus store_get_linear(const Store *store, const char *key, const char **out_value);

/* Обход всех пар. Порядок — порядок ячеек таблицы, то есть произвольный.
   Функция visit вызывается для каждой пары; context передаётся ей без изменений. */
void store_for_each(const Store *store,
                    void (*visit)(const char *key, const char *value, void *context),
                    void *context);

void store_stats(const Store *store, StoreStats *stats);

const char *store_status_text(StoreStatus status);

#endif /* STORE_H */
