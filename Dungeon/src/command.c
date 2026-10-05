#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "command.h"

static void discard_rest_of_line(const char *buffer) {
    if (strchr(buffer, '\n') != NULL) {
        return;
    }
    int c = getchar();
    while (c != EOF && c != '\n') {
        c = getchar();
    }
}

static void split_words(Command *command) {
    command->count = 0;
    char *p = command->line;
    while (*p != '\0' && command->count < COMMAND_WORDS_MAX) {
        while (*p != '\0' && isspace((unsigned char)*p)) {
            *p = '\0';
            ++p;
        }
        if (*p == '\0') {
            break;
        }

        command->word[command->count] = p;
        ++command->count;

        while (*p != '\0' && !isspace((unsigned char)*p)) {
            ++p;
        }
    }
}

void command_read(Command *command) {
    command->count = 0;
    command->eof = 0;
    if (fgets(command->line, COMMAND_LINE_MAX, stdin) == NULL) {
        command->eof = 1;
        return;
    }
    discard_rest_of_line(command->line);
    split_words(command);

    if (command->count > 0) {
        for (int i = 0; command->word[0][i] != '\0'; ++i) {
            command->word[0][i] = (char)tolower((unsigned char)command->word[0][i]);
        }
    }
}

const char *command_name(const Command *command) {
    if (command->count == 0) {
        return "";
    }
    return command->word[0];
}

const char *command_argument(const Command *command, const int index) {
    if (index < 1 || index >= command->count) {
        return NULL;
    }
    return command->word[index];
}
