#ifndef STORE_H
#define STORE_H

#include <stddef.h>

/* Практика 7: ключи и значения любой длины, таблица растёт сама.

   Когда заполненность переваливает за семьдесят процентов, создаётся
   вдвое большая таблица и все ключи переносятся заново: номер ячейки
   зависит от размера таблицы, поэтому перенести их по прежним номерам
   нельзя. Заодно перестроение убирает пометки об удалении. */

#define STORE_FIRST_CAPACITY 1024 /* степень двойки: номер ячейки берётся маской */
#define STORE_LOAD_PERCENT   70

typedef enum {
    STORE_OK = 0,
    STORE_ERR_ARG,
    STORE_ERR_MEMORY,
    STORE_ERR_NOT_FOUND
} StoreStatus;

/* Три состояния ячейки. Третье обязательно: если пометить удалённую
   ячейку пустой, поиск остановится на ней и не найдёт ключи,
   прошедшие при вставке дальше по цепочке проб. */
typedef enum {
    CELL_EMPTY = 0,
    CELL_USED,
    CELL_DELETED
} CellState;

typedef struct {
    CellState state;
    char *key;   /* в куче, по длине ключа; у пустой и удалённой ячейки NULL */
    char *value; /* в куче, по длине значения */
} Cell;

typedef struct {
    Cell *cells;       /* таблица в куче; NULL, если хранилище закрыто */
    int capacity;      /* вместимость, степень двойки */
    int count;         /* занятых ячеек */
    int deleted;       /* ячеек с пометкой «удалено» */
    long rehash_count; /* сколько раз таблица перестраивалась */
} Store;

typedef struct {
    int count;
    int capacity;
    int deleted;
    int displaced;     /* ключей, лежащих не в своей домашней ячейке */
    int longest_probe; /* самая длинная цепочка проб до ключа */
    long rehash_count;
} StoreStats;

/* Создаёт пустое хранилище. Возвращает 0, если памяти не хватило. */
int store_init(Store *store);

/* Освобождает все ключи, значения и таблицу. Повторный вызов безопасен. */
void store_destroy(Store *store);

/* Хеш строки. Сумма кодов символов не годится: у слов из одних
   и тех же букв она одинакова. */
unsigned long store_hash(const char *key);

/* Домашняя ячейка ключа — та, с которой начинается поиск. Зависит
   от вместимости, поэтому при росте таблицы ключи раскладываются заново. */
int store_home(const Store *store, const char *key);

/* Кладёт значение по ключу. Ключ и значение копируются в кучу.
   Через was_present сообщает, был ли ключ до вызова; параметр можно не передавать. */
StoreStatus store_put(Store *store, const char *key, const char *value, int *was_present);
StoreStatus store_get(const Store *store, const char *key, const char **out_value);
StoreStatus store_remove(Store *store, const char *key);
int store_count(const Store *store);

/* Поиск перебором всех ячеек — для сравнения скорости с хешированием. */
StoreStatus store_get_linear(const Store *store, const char *key, const char **out_value);

/* Обход всех пар в порядке ячеек таблицы, то есть в произвольном. */
void store_for_each(const Store *store,
                    void (*visit)(const char *key, const char *value, void *context),
                    void *context);

void store_stats(const Store *store, StoreStats *stats);
const char *store_status_text(StoreStatus status);

#endif /* STORE_H */
