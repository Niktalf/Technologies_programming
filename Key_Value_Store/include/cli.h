#ifndef CLI_H
#define CLI_H

#include "store.h"

/* Выполняет одну команду, заданную набором слов.
   Возвращает 0, если работу пора завершать, иначе 1. */
int cli_execute(Store *store, int argc, char *argv[]);

void cli_print_help(void);

/* Интерактивный цикл. */
void cli_run_interactive(Store *store);

#endif /* CLI_H */
