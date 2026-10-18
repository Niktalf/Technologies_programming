#include <stdio.h>

#include "cli.h"
#include "session.h"

int main(int argc, char *argv[]) {
    /* Содержимое хранилищ лежит в куче, поэтому сессия — обычная локальная
       переменная. Глобальной она не становится: адрес передаётся явно. */
    Session session;
    if (!session_init(&session)) {
        fprintf(stderr, "Not enough memory for the stores\n");
        return 1;
    }

    /* С аргументами: одна команда и выход, код возврата 1, если команда
       не выполнилась. Без аргументов: интерактивный режим. */
    int result = 0;
    if (argc > 1) {
        result = cli_execute(&session, argc - 1, argv + 1) == CLI_ERROR ? 1 : 0;
    } else {
        cli_run_interactive(&session);
    }

    /* Выделили — освобождаем. */
    session_destroy(&session);
    return result;
}
