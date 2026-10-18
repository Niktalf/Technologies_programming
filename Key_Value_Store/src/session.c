#include <stddef.h>

#include "session.h"

int session_init(Session *session) {
    if (session == NULL) {
        return 0;
    }
    session->current = 0;
    int created = 0;
    while (created < SESSION_STORES && store_init(&session->store[created])) {
        ++created;
    }
    if (created == SESSION_STORES) {
        return 1;
    }
    /* Что успели создать — освобождаем, иначе утечёт. */
    for (int i = 0; i < created; ++i) {
        store_destroy(&session->store[i]);
    }
    return 0;
}

void session_destroy(Session *session) {
    if (session == NULL) {
        return;
    }
    for (int i = 0; i < SESSION_STORES; ++i) {
        store_destroy(&session->store[i]);
    }
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
    /* При двух хранилищах «другое» — это единица минус текущее. */
    return &session->store[1 - session->current];
}

int session_select(Session *session, int number) {
    if (session == NULL || number < 1 || number > SESSION_STORES) {
        return 0;
    }
    session->current = number - 1;
    return 1;
}
