#include "cli.h"
#include "store.h"

int main(int argc, char *argv[]) {
    Store store;
    store_init(&store);

    /* С аргументами: одна команда и выход. Без аргументов: интерактивный режим.
       Нулевой аргумент — имя программы, команда начинается с первого. */
    if (argc > 1) {
        cli_execute(&store, argc - 1, argv + 1);
        return 0;
    }
    cli_run_interactive(&store);
    return 0;
}
