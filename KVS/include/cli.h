#ifndef CLI_H
#define CLI_H

#include "session.h"

int cli_execute(Session *session, int argc, char **argv);
void cli_print_help();
void cli_run_interactive(Session *session);

#endif // CLI_H
