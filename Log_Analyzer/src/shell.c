#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "shell.h"

#define SHELL_LINE_MAX 64

static void print_help(void) {
    printf("Available commands:\n");
    printf("  count - number of lines in the log\n");
    printf("  head  - first ten lines\n");
    printf("  tail  - last ten lines\n");
    printf("  help  - this list\n");
    printf("  quit  - exit\n");
}

/* Дочитывает и выбрасывает остаток строки до перевода строки или конца ввода. */
static void skip_rest_of_line(void) {
    int c = getchar();
    while (c != '\n' && c != EOF) {
        c = getchar();
    }
}

/* Читает команду: обрезает пробелы по краям и приводит к нижнему регистру.
   Возвращает 0, если ввод закончился. */
static int read_command(char *buffer, size_t size) {
    if (fgets(buffer, (int)size, stdin) == NULL) {
        return 0;
    }
    if (strchr(buffer, '\n') == NULL) {
        skip_rest_of_line();
    }

    size_t length = strlen(buffer);
    while (length > 0 && isspace((unsigned char)buffer[length - 1])) {
        --length;
        buffer[length] = '\0';
    }

    size_t start = 0;
    while (buffer[start] != '\0' && isspace((unsigned char)buffer[start])) {
        ++start;
    }
    if (start > 0) {
        memmove(buffer, buffer + start, length - start + 1);
    }

    for (size_t i = 0; buffer[i] != '\0'; ++i) {
        buffer[i] = (char)tolower((unsigned char)buffer[i]);
    }
    return 1;
}

void shell_run(LogFile *log) {
    printf("Lines loaded: %ld\n", log->total);
    if (log->truncated > 0) {
        printf("Lines too long and truncated: %ld\n", log->truncated);
    }
    printf("Type help for the list of commands.\n");

    /* Конец ввода и команда quit сводятся к одному флагу: у цикла одно
       условие выхода, без break и continue. */
    int running = 1;
    while (running) {
        printf("> ");
        fflush(stdout);

        char command[SHELL_LINE_MAX];
        if (!read_command(command, sizeof command)) {
            running = 0;
        } else if (command[0] == '\0') {
            /* Пустая строка — не ошибка: просто ждём следующую команду. */
        } else if (strcmp(command, "count") == 0) {
            printf("Lines in the log: %ld\n", log->total);
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
            printf("Type help for the list of commands.\n");
        }
    }
    printf("Done.\n");
}
