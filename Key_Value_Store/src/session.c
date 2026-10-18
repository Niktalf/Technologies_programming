#include <stddef.h>
#include <string.h>

#include "session.h"

/* Запоминает путь, если он помещается в буфер; иначе KV_ERR_ARG. */
static KvStatus remember_path(char *target, const char *path) {
    size_t length = strlen(path);
    if (length >= SESSION_PATH_MAX) {
        return KV_ERR_ARG;
    }
    memcpy(target, path, length + 1);
    return KV_OK;
}

KvStatus session_init(Session *session, const char *first, const char *second) {
    if (session == NULL || first == NULL || second == NULL) {
        return KV_ERR_ARG;
    }
    session->current = 0;
    for (int i = 0; i < SESSION_STORES; ++i) {
        session->store[i] = NULL;
        session->path[i][0] = '\0';
    }
    const char *paths[SESSION_STORES] = { first, second };
    KvStatus status = KV_OK;
    for (int i = 0; i < SESSION_STORES && status == KV_OK; ++i) {
        status = remember_path(session->path[i], paths[i]);
        if (status == KV_OK) {
            status = kv_open(paths[i], &session->store[i]);
        }
    }
    if (status != KV_OK) {
        session_destroy(session); /* что успели открыть — закрываем, иначе утечёт */
    }
    return status;
}

void session_destroy(Session *session) {
    if (session == NULL) {
        return;
    }
    for (int i = 0; i < SESSION_STORES; ++i) {
        kv_close(session->store[i]); /* закрыть NULL безопасно */
        session->store[i] = NULL;
    }
}

KvStore *session_current(Session *session) {
    if (session == NULL) {
        return NULL;
    }
    return session->store[session->current];
}

KvStore *session_other(Session *session) {
    if (session == NULL) {
        return NULL;
    }
    /* При двух хранилищах «другое» — это единица минус текущее. */
    return session->store[1 - session->current];
}

const char *session_current_path(const Session *session) {
    if (session == NULL) {
        return "";
    }
    return session->path[session->current];
}

KvStatus session_select(Session *session, int number) {
    if (session == NULL || number < 1 || number > SESSION_STORES) {
        return KV_ERR_ARG;
    }
    session->current = number - 1;
    return KV_OK;
}

void session_close(Session *session) {
    if (session == NULL) {
        return;
    }
    kv_close(session->store[session->current]);
    session->store[session->current] = NULL;
}

KvStatus session_open(Session *session, const char *path) {
    if (session == NULL || path == NULL) {
        return KV_ERR_ARG;
    }
    session_close(session);
    KvStatus status = remember_path(session->path[session->current], path);
    if (status != KV_OK) {
        return status;
    }
    return kv_open(path, &session->store[session->current]);
}
