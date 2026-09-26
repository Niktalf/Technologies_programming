#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "shell.h"

#define SHELL_LINE_MAX 64

static void print_help(void)
{
    printf("Available commands:\n");
    printf("  count - number of lines in the log\n");
    printf("  head  - first ten lines\n");
    printf("  tail  - last ten lines\n");
    printf("  help  - this list\n");
    printf("  quit  - exit\n");
}

static int read_command(char *buffer, const size_t size)
{
    if (fgets(buffer, (int)size, stdin) == NULL) {
        return 0;
    }
    if (strchr(buffer, '\n') == NULL) {
        int c;
        while ((c = getchar()) != EOF && c != '\n') {
            /* пусто */
        }
    }
    size_t length = strlen(buffer);
    while (length > 0 && isspace((unsigned char)buffer[length - 1])) {
        buffer[--length] = '\0';
    }

    size_t start = 0;
    while (buffer[start] != '\0' && isspace((unsigned char)buffer[start])) {
        ++start;
    }
    if (start > 0) {
        memmove(buffer, buffer + start, strlen(buffer + start) + 1);
    }
    for (size_t i = 0; buffer[i] != '\0'; ++i) {
        buffer[i] = (char)tolower((unsigned char)buffer[i]);
    }
    return 1;
}

void shell_run(const LogFile *log)
{
    char command[SHELL_LINE_MAX];
    int running = 1;

    printf("Loaded lines: %ld\n", log->total);
    if (log->truncated > 0) {
        printf("Truncated too long lines: %ld\n", log->truncated);
    }
    printf("Type help for a list of commands.\n");

    while (running) {
        printf("> ");
        fflush(stdout);

        if (!read_command(command, sizeof command)) {
            break;
        }
        if (command[0] == '\0') {
            continue;
        }
        if (strcmp(command, "count") == 0) {
            printf("Count of lines in the log: %ld\n", log->total);
        } else if (strcmp(command, "head") == 0) {
            logfile_print_head(log, LOGFILE_HEAD_SIZE);
        } else if (strcmp(command, "tail") == 0) {
            logfile_print_tail(log, LOGFILE_TAIL_SIZE);
        } else if (strcmp(command, "help") == 0) {
            print_help();
        } else if (strcmp(command, "quit") == 0) {
            running = 0;
        } else {
            printf("Unknown command: %s\n", command);
        }
    }
    printf("The work is completed.\n");
}
