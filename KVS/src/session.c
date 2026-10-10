#include <stddef.h>

#include "session.h"

void session_init(Session *session) {
    if (session == NULL) {
        return;
    }
    for (int i = 0; i < SESSION_STORES; ++i) {
        store_init(&session->store[i]);
    }
    session->current = 0;
}

Store *session_current(Session *session) {
    if (session == NULL) {
        return NULL;
    }
    return &session->store[session->current];
}

Store *session_other(Session *session) {
    if (session == NULL) {
        return NULL;
    }
    return &session->store[1 - session->current];
}

int session_select(Session *session, const int number) {
    if (session == NULL || number < 1 || number > SESSION_STORES) {
        return 0;
    }
    session->current = number - 1;
    return 1;
}
