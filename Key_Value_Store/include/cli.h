#ifndef CLI_H
#define CLI_H

#include "session.h"
#include "store.h"

/* Результат одной команды. Скрипту мало знать «продолжать или нет»:
   ему нужно отличить ошибку, чтобы сообщить номер строки. */
#define CLI_QUIT  0 /* команда выхода */
#define CLI_OK    1 /* выполнено, в том числе «ключ не найден» — это ответ, а не ошибка */
#define CLI_ERROR 2 /* неизвестная команда, нет аргументов, не хватило памяти */

/* Выполняет одну команду, заданную набором слов. Возвращает CLI_QUIT,
   CLI_OK или CLI_ERROR. */
int cli_execute(Session *session, int argc, char *argv[]);

void cli_print_help(void);

/* Интерактивный цикл. */
void cli_run_interactive(Session *session);

#endif /* CLI_H */
