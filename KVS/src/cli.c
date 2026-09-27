#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "cli.h"
#include "record.h"

#define CLI_LINE_MAX  256
#define CLI_ARGS_MAX  8

void cli_print_help(void)
{
    printf("Available commands:\n");
    printf("  put <key> <value>     - save a pair\n");
    printf("  get <key>             - get a value\n");
    printf("  del <key>             - delete a pair\n");
    printf("  list                  - list all keys\n");
    printf("  record <key> <value>  - collect a log entry and verify it\n");
    printf("  help                  - this list\n");
    printf("  quit                  - exit\n");
}

static void report(const StoreStatus status)
{
    fprintf(stderr, "Error: %s\n", store_status_text(status));
}

static void demo_record(const char *key, const char *value)
{
    uint8_t buffer[512];
    size_t size = 0;

    RecordStatus status = record_build(buffer, sizeof buffer,
                                       record_flags_set(0, RECORD_FLAG_PUT), key, value, &size);
    if (status != RECORD_OK) {
        fprintf(stderr, "Build error: %s\n", record_status_text(status));
        return;
    }

    printf("Record size: %zu bytes (header %d + key %zu + value %zu)\n",
           size, RECORD_HEADER_SIZE, strlen(key), strlen(value));
    printf("bytes: ");
    for (size_t i = 0; i < size; ++i) {
        printf("%02X ", (unsigned)buffer[i]);
    }
    printf("\n");
    RecordHeader header;
    status = record_parse(buffer, size, &header);
    printf("Parsing: %s\n", record_status_text(status));
    record_flags_print(header.flags);
    printf("Key length: %u, value length: %u, sum: 0x%02X\n",
           (unsigned)header.key_length, (unsigned)header.value_length,
           (unsigned)header.checksum);

    buffer[size - 1] = (uint8_t)(buffer[size - 1] ^ 0x01u);
    status = record_parse(buffer, size, &header);
    printf("After corrupting the last byte: %s\n", record_status_text(status));
    buffer[size - 1] = (uint8_t)(buffer[size - 1] ^ 0x01u);

    buffer[2] = (uint8_t)(buffer[2] ^ 0x01u);
    status = record_parse(buffer, size, &header);
    printf("After key length corruption: %s\n", record_status_text(status));
    buffer[2] = (uint8_t)(buffer[2] ^ 0x01u);
}

int cli_execute(Store *store, int argc, char **argv)
{
    if (argc == 0) {
        return 1;
    }

    if (strcmp(argv[0], "put") == 0) {
        if (argc < 3) {
            fprintf(stderr, "put requires a key and a value\n");
            return 1;
        }

        int was_present = 0;
        const StoreStatus status = store_put(store, argv[1], argv[2], &was_present);
        if (status != STORE_OK) {
            report(status);
        } else {
            printf(was_present ? "updated\n" : "added\n");
        }
        return 1;
    }

    if (strcmp(argv[0], "get") == 0) {
        if (argc < 2) {
            fprintf(stderr, "get requires a key\n");
            return 1;
        }
        const char *value = NULL;
        const StoreStatus status = store_get(store, argv[1], &value);
        if (status != STORE_OK) {
            report(status);
        } else {
            printf("%s\n", value);
        }
        return 1;
    }

    if (strcmp(argv[0], "del") == 0) {
        if (argc < 2) {
            fprintf(stderr, "del requires a key\n");
            return 1;
        }
        const StoreStatus status = store_remove(store, argv[1]);
        if (status != STORE_OK) {
            report(status);
        } else {
            printf("deleted\n");
        }
        return 1;
    }

    if (strcmp(argv[0], "list") == 0) {
        const int total = store_count(store);

        if (total == 0) {
            printf("The storage is empty.\n");
        }
        for (int i = 0; i < total; ++i) {
            const char *key = NULL;
            const char *value = NULL;

            if (store_at(store, i, &key, &value) == STORE_OK) {
                printf("%s = %s\n", key, value);
            }
        }
        return 1;
    }

    if (strcmp(argv[0], "record") == 0) {
        if (argc < 3) {
            fprintf(stderr, "record requires a key and a value\n");
            return 1;
        }
        demo_record(argv[1], argv[2]);
        return 1;
    }

    if (strcmp(argv[0], "help") == 0) {
        cli_print_help();
        return 1;
    }

    if (strcmp(argv[0], "quit") == 0 || strcmp(argv[0], "exit") == 0) {
        return 0;
    }

    fprintf(stderr, "Unknown command: %s\n", argv[0]);
    return 1;
}

static int split_words(char *line, char **argv, const int max_args)
{
    int argc = 0;
    char *p = line;

    while (*p != '\0' && argc < max_args) {
        while (*p != '\0' && isspace((unsigned char)*p)) {
            *p++ = '\0';
        }
        if (*p == '\0') {
            break;
        }
        argv[argc++] = p;
        while (*p != '\0' && !isspace((unsigned char)*p)) {
            ++p;
        }
    }
    return argc;
}

void cli_run_interactive(Store *store)
{
    char line[CLI_LINE_MAX];
    char *argv[CLI_ARGS_MAX];
    int running = 1;

    printf("Key-value store. Type help for a list of commands.\n");

    while (running) {
        printf("> ");
        fflush(stdout);

        if (fgets(line, (int)sizeof line, stdin) == NULL) {
            break;
        }
        if (strchr(line, '\n') == NULL) {
            int c;
            while ((c = getchar()) != EOF && c != '\n') {}
        }
        const int argc = split_words(line, argv, CLI_ARGS_MAX);
        if (argc == 0) {
            continue;
        }
        running = cli_execute(store, argc, argv);
    }
    printf("The work is completed.\n");
}
