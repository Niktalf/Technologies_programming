#include <stdio.h>
#include <string.h>

#include "cli.h"
#include "session.h"

/* Журналы по умолчанию лежат в текущем каталоге. Первый можно
   заменить ключом -f: ./kvstore -f book.db put name Nikita */
#define DEFAULT_FIRST  "store1.db"
#define DEFAULT_SECOND "store2.db"

int main(int argc, char *argv[]) {
    const char *first = DEFAULT_FIRST;
    int shift = 1;
    if (argc >= 3 && strcmp(argv[1], "-f") == 0) {
        first = argv[2];
        shift = 3;
    }

    Session session;
    KvStatus status = session_init(&session, first, DEFAULT_SECOND);
    if (status != KV_OK) {
        fprintf(stderr, "Cannot open the stores: %s\n", kv_status_text(status));
        return 1;
    }

    /* С командой в аргументах: выполнить её и выйти. Без неё: интерактивный режим.
       Поскольку данные теперь в журнале, можно положить ключ одним запуском
       и прочитать другим. */
    int result = 0;
    if (argc > shift) {
        result = cli_execute(&session, argc - shift, argv + shift) == CLI_ERROR ? 1 : 0;
    } else {
        cli_run_interactive(&session);
    }

    session_destroy(&session);
    return result;
}
