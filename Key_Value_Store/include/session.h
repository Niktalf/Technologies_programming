#ifndef SESSION_H
#define SESSION_H

#include "store.h"

/* Практика 5: несколько независимых хранилищ в одной программе.

   Ни одно хранилище не лежит в глобальной переменной: все функции
   получают адрес того хранилища, с которым работают. Поэтому хранилищ
   может быть сколько угодно, и они друг о друге не знают. */

#define SESSION_STORES 2

typedef struct {
    Store store[SESSION_STORES];
    int current; /* номер текущего хранилища, от 0 */
} Session;

/* Содержимое хранилищ лежит в куче, и сама сессия невелика. */

/* Создаёт оба хранилища. Возвращает 1 при успехе; при неудаче то,
   что успели создать, освобождено. */
int session_init(Session *session);

/* Освобождает оба хранилища. */
void session_destroy(Session *session);

/* Текущее хранилище. Возвращает адрес, а не копию:
   копия весила бы мегабайты, и изменения в неё не вернулись бы. */
Store *session_current(Session *session);

/* Другое хранилище — то, которое сейчас не выбрано. */
Store *session_other(Session *session);

/* Переключает текущее хранилище. Возвращает 1, если номер подходит. */
int session_select(Session *session, int number);

#endif /* SESSION_H */
