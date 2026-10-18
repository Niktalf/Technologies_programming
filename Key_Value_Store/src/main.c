#include <stdio.h>
#include <stdlib.h>

#include "cli.h"
#include "session.h"

int main(int argc, char *argv[]) {
    /* Два хранилища по 2,7 МБ в стек не поместятся, поэтому сессия
       выделяется в куче. Глобальной переменной она не становится:
       адрес хранится в локальной переменной и передаётся в функции явно. */
    Session *session = malloc(sizeof *session);
    if (session == NULL) {
        fprintf(stderr, "Not enough memory for the stores\n");
        return 1;
    }
    session_init(session);

    /* С аргументами: одна команда и выход, код возврата 1, если команда
       не выполнилась. Без аргументов: интерактивный режим. */
    int result = 0;
    if (argc > 1) {
        result = cli_execute(session, argc - 1, argv + 1) == CLI_ERROR ? 1 : 0;
    } else {
        cli_run_interactive(session);
    }

    free(session);
    return result;
}
