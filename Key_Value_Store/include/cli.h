#ifndef CLI_H
#define CLI_H

#include "session.h"
#include "store.h"

/* Выполняет одну команду, заданную набором слов, над текущим хранилищем
   сессии. Возвращает 0, если работу пора завершать, иначе 1. */
int cli_execute(Session *session, int argc, char *argv[]);

void cli_print_help(void);

/* Интерактивный цикл. */
void cli_run_interactive(Session *session);

#endif /* CLI_H */
