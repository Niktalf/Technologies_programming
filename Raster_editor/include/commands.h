#ifndef COMMANDS_H
#define COMMANDS_H

#include "editor.h"

typedef enum {
    OP_UNKNOWN = 0,
    OP_INFO,
    OP_HELP,
    OP_QUIT,
    OP_EMPTY,
    OP_EOF
} Operation;

Operation operation_from_word(const char *word);

int operation_execute(Operation operation, const Editor *editor, const char *raw_word);

void commands_print_help();

#endif // COMMANDS_H
