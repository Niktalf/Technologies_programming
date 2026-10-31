#ifndef KVSTORE_H
#define KVSTORE_H

/* Практика 8: публичный интерфейс библиотеки.

   Это единственный заголовок, который включает чужой код. Здесь нет
   ни хеш-таблицы, ни формата записи, ни смещений в файле: всё это
   внутреннее дело библиотеки, и менять его можно, не трогая клиентов.

   Данные переживают перезапуск: каждая операция дописывается в журнал,
   при открытии журнал перечитывается и таблица собирается заново.
   Если программу сняли на середине записи, повреждённый хвост журнала
   отбрасывается, а всё записанное раньше сохраняется. */

typedef struct KvStore KvStore;

/* Неполный тип: клиент знает имя, но не размер и не поля. Поэтому он не может
   ни создать объект сам, ни скопировать его, ни залезть внутрь — только
   держать указатель и звать функции библиотеки. */

/* Общее перечисление кодов ошибок публичного интерфейса. */
typedef enum {
    KV_OK = 0,
    KV_ERR_ARG,       /* неверный аргумент: пустой ключ, слишком длинное значение */
    KV_ERR_MEMORY,    /* не хватило памяти */
    KV_ERR_NOT_FOUND, /* ключа нет — это ответ, а не поломка */
    KV_ERR_OPEN,      /* журнал не открылся */
    KV_ERR_IO         /* ошибка чтения или записи журнала */
} KvStatus;

const char *kv_status_text(KvStatus status);

/* Открывает базу по пути к журналу. Файла может не быть — тогда
   создаётся пустая база. Повреждённый хвост журнала отбрасывается.
   При ошибке *out_store равен NULL и ничего не остаётся открытым. */
KvStatus kv_open(const char *path, KvStore **out_store);

/* Закрывает базу. Передать NULL безопасно. */
void kv_close(KvStore *store);

/* Ключ — от 1 до 255 байт, значение — до 4096 байт. Каждая операция
   сначала дописывается в журнал и сбрасывается на диск, потом меняет память. */
KvStatus kv_put(KvStore *store, const char *key, const char *value);
KvStatus kv_get(const KvStore *store, const char *key, const char **out_value);
KvStatus kv_delete(KvStore *store, const char *key);

int kv_count(const KvStore *store);

/* Сколько записей в журнале и сколько из них уже не нужны: по этой доле
   на практике 15 будет запускаться уплотнение. */
long kv_journal_records(const KvStore *store);
long kv_journal_garbage(const KvStore *store);

/* Сколько записей отброшено при открытии как повреждённые. */
long kv_recovered_broken(const KvStore *store);

void kv_for_each(const KvStore *store,
                 void (*visit)(const char *key, const char *value, void *context),
                 void *context);

#endif /* KVSTORE_H */
