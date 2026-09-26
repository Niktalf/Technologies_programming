#include <stdio.h>
#include <string.h>

#include "commands.h"

Operation operation_from_word(const char *word)
{
    if (word == NULL || word[0] == '\0') {
        return OP_EMPTY;
    }
    if (strcmp(word, "info") == 0) {
        return OP_INFO;
    }
    if (strcmp(word, "help") == 0) {
        return OP_HELP;
    }
    if (strcmp(word, "quit") == 0 || strcmp(word, "exit") == 0) {
        return OP_QUIT;
    }
    return OP_UNKNOWN;
}

void commands_print_help()
{
    printf("Available operations:\n");
    printf("  info  - information about the current image\n");
    printf("  help  - this list\n");
    printf("  quit  - exit\n");
}

int operation_execute(const Operation operation, const Editor *editor, const char *raw_word)
{
    switch (operation) {
    case OP_INFO:
        editor_print_info(editor);
        return 1;
    case OP_HELP:
        commands_print_help();
        return 1;
    case OP_EMPTY:
        return 1;
    case OP_QUIT:
    case OP_EOF:
        return 0;
    case OP_UNKNOWN:
    default:
        fprintf(stderr, "Unknown operation: %s\n", raw_word != NULL ? raw_word : "");
        fprintf(stderr, "Type help to see the list of operations.\n");
        return 1;
    }
}
