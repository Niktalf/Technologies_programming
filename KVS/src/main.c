#include "cli.h"
#include "store.h"

int main(int argc, char **argv)
{
    Store store;

    store_init(&store);

    if (argc > 1) {
        cli_execute(&store, argc - 1, argv + 1);
        return 0;
    }
    cli_run_interactive(&store);
    return 0;
}
