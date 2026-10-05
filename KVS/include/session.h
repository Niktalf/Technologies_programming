#ifndef SESSION_H
#define SESSION_H

#include "store.h"

#define SESSION_STORES 2

typedef struct {
    Store store[SESSION_STORES];
    int   current;
} Session;

void session_init(Session *session);

Store *session_current(Session *session);
Store *session_other(Session *session);

int session_select(Session *session, int number);

#endif // SESSION_H
