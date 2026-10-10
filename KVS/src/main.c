#include "cli.h"
#include "session.h"

static Session session;

int main(int argc, char **argv)
{
    session_init(&session);
    if (argc > 1) {
        cli_execute(&session, argc - 1, argv + 1);
        return 0;
    }

    cli_run_interactive(&session);
    return 0;
}
