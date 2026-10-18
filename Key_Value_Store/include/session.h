#ifndef SESSION_H
#define SESSION_H

#include "kvstore.h"

/* Практика 5: несколько независимых хранилищ в одной программе.

   Практика 8: каждое хранилище — база библиотеки со своим журналом
   на диске. Консольная программа больше не знает, как устроена таблица:
   она видит только тип KvStore из публичного заголовка. */

#define SESSION_STORES   2
#define SESSION_PATH_MAX 256

typedef struct {
    KvStore *store[SESSION_STORES]; /* NULL — хранилище закрыто */
    char path[SESSION_STORES][SESSION_PATH_MAX];
    int current;                    /* номер текущего хранилища, от 0 */
} Session;

/* Открывает оба хранилища по путям к журналам. При ошибке ничего
   не остаётся открытым. */
KvStatus session_init(Session *session, const char *first, const char *second);
void session_destroy(Session *session);

/* Текущее и другое хранилище. NULL, если оно закрыто. */
KvStore *session_current(Session *session);
KvStore *session_other(Session *session);
const char *session_current_path(const Session *session);

/* Переключает текущее хранилище. KV_ERR_ARG, если номер не подходит. */
KvStatus session_select(Session *session, int number);

/* Закрывает текущее хранилище: память освобождается, журнал остаётся на диске. */
void session_close(Session *session);

/* Закрывает текущее хранилище и открывает на его месте другой журнал.
   Если открыть не удалось, текущее остаётся закрытым. */
KvStatus session_open(Session *session, const char *path);

#endif /* SESSION_H */
