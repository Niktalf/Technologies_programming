#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "cli.h"

#define CLI_LINE_MAX 256
#define CLI_ARGS_MAX 8

/* Все ответы на команды, включая сообщения об ошибках, идут в обычный вывод:
   они часть диалога и должны стоять в нём по порядку, сразу после приглашения. */

void cli_print_help(void) {
    printf("Available commands:\n");
    printf("  put <key> <value>  - store a pair\n");
    printf("  get <key>          - get the value\n");
    printf("  del <key>          - delete the pair\n");
    printf("  list               - list all pairs\n");
    printf("  help               - this list\n");
    printf("  quit               - exit\n");
}

static void report(StoreStatus status) {
    printf("Error: %s\n", store_status_text(status));
}

static void command_put(Store *store, int argc, char *argv[]) {
    if (argc < 3) {
        printf("put needs a key and a value\n");
        return;
    }
    int was_present = 0;
    StoreStatus status = store_put(store, argv[1], argv[2], &was_present);
    if (status != STORE_OK) {
        report(status);
        return;
    }
    printf(was_present ? "updated\n" : "added\n");
}

static void command_get(const Store *store, int argc, char *argv[]) {
    if (argc < 2) {
        printf("get needs a key\n");
        return;
    }
    const char *value = NULL;
    StoreStatus status = store_get(store, argv[1], &value);
    if (status != STORE_OK) {
        report(status);
        return;
    }
    printf("%s\n", value);
}

static void command_del(Store *store, int argc, char *argv[]) {
    if (argc < 2) {
        printf("del needs a key\n");
        return;
    }
    StoreStatus status = store_remove(store, argv[1]);
    if (status != STORE_OK) {
        report(status);
        return;
    }
    printf("deleted\n");
}

static void command_list(const Store *store) {
    int total = store_count(store);
    if (total == 0) {
        printf("The store is empty.\n");
        return;
    }
    for (int i = 0; i < total; ++i) {
        const char *key = NULL;
        const char *value = NULL;
        if (store_at(store, i, &key, &value) == STORE_OK) {
            printf("%s = %s\n", key, value);
        }
    }
}

int cli_execute(Store *store, int argc, char *argv[]) {
    if (argc == 0) {
        return 1;
    }
    const char *name = argv[0];
    if (strcmp(name, "put") == 0) {
        command_put(store, argc, argv);
    } else if (strcmp(name, "get") == 0) {
        command_get(store, argc, argv);
    } else if (strcmp(name, "del") == 0) {
        command_del(store, argc, argv);
    } else if (strcmp(name, "list") == 0) {
        command_list(store);
    } else if (strcmp(name, "help") == 0) {
        cli_print_help();
    } else if (strcmp(name, "quit") == 0 || strcmp(name, "exit") == 0) {
        return 0;
    } else {
        printf("Unknown command: %s\n", name);
        printf("Type help for the list of commands.\n");
    }
    return 1;
}

/* Разбивает строку на слова по пробелам: пробелы заменяются нулями, а в argv
   попадают указатели на начала слов внутри самой строки. Слова сверх max_args
   отбрасываются. Кавычки появятся на практике 6. */
static int split_words(char *line, char *argv[], int max_args) {
    int argc = 0;
    char *p = line;
    while (argc < max_args) {
        while (*p != '\0' && isspace((unsigned char)*p)) {
            *p = '\0';
            ++p;
        }
        if (*p == '\0') {
            return argc;
        }
        argv[argc] = p;
        ++argc;
        while (*p != '\0' && !isspace((unsigned char)*p)) {
            ++p;
        }
    }
    return argc;
}

/* Дочитывает и выбрасывает остаток строки до перевода строки или конца ввода. */
static void skip_rest_of_line(void) {
    int c = getchar();
    while (c != '\n' && c != EOF) {
        c = getchar();
    }
}

void cli_run_interactive(Store *store) {
    printf("Key-value store. Type help for the list of commands.\n");

    /* Конец ввода и команда quit сводятся к одному флагу: у цикла одно
       условие выхода. Пустая строка даёт ноль слов, и cli_execute её пропускает. */
    int running = 1;
    while (running) {
        printf("> ");
        fflush(stdout);

        char line[CLI_LINE_MAX];
        if (fgets(line, (int)sizeof line, stdin) == NULL) {
            running = 0;
        } else {
            /* Хвост слишком длинной строки не должен стать следующей командой. */
            if (strchr(line, '\n') == NULL) {
                skip_rest_of_line();
            }
            char *argv[CLI_ARGS_MAX];
            int argc = split_words(line, argv, CLI_ARGS_MAX);
            running = cli_execute(store, argc, argv);
        }
    }
    printf("Done.\n");
}
