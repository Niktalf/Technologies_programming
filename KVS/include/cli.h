#ifndef CLI_H
#define CLI_H

#include "store.h"

int cli_execute(Store *store, int argc, char **argv);

void cli_print_help();

void cli_run_interactive(Store *store);

#endif // CLI_H
