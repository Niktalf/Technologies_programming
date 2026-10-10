#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#include "cli.h"
#include "range.h"
#include "record.h"

#define CLI_LINE_MAX  256
#define CLI_ARGS_MAX  8

void cli_print_help() {
    printf("Available commands:\n");
    printf("  put <key> <value>     - save a pair\n");
    printf("  get <key>             - get a value\n");
    printf("  del <key>             - delete a pair\n");
    printf("  list                  - list all keys\n");
    printf("  sorted                - all pairs in ascending order of the key\n");
    printf("  range <from> <to>     - pairs with keys in the range, boundaries are included\n");
    printf("  use <1|2>             - switch to another storage\n");
    printf("  which                 - which storage is selected\n");
    printf("  copy <key>            - copy the pair to another storage\n");
    printf("  sizes                 - why is storage transferred to\n");
    printf("  stats                 - table occupancy and collisions\n");
    printf("  bench [n]             - compare hash and iteration on n keys\n");
    printf("  collide               - removing samples from the chain\n");
    printf("  record <key> <value>  - collect a log entry and verify it\n");
    printf("  help                  - this list\n");
    printf("  quit                  - exit\n");
}

static void report(const StoreStatus status) {
    fprintf(stderr, "Error: %s\n", store_status_text(status));
}

static void demo_record(const char *key, const char *value) {
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
    printf("Bytes: ");
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

static void print_pair(const char *key, const char *value, void *context) {
    (void)context;
    printf("%s = %s\n", key, value);
}

static void print_stats(const Store *store) {
    StoreStats stats;

    store_stats(store, &stats);
    printf("Keys: %d from %d (%d%%)\n",
           stats.count, stats.capacity, stats.count * 100 / stats.capacity);
    printf("Cell marked \"deleted\": %d\n", stats.deleted);
    printf("Keys aren't in their cell: %d\n", stats.displaced);
    printf("Rather long chain of samples: %d\n", stats.longest_probe);
}

static Store bench_store;

static double seconds_since(const clock_t start) {
    return (double)(clock() - start) / CLOCKS_PER_SEC;
}

static void run_bench(const long n) {
    if (n <= 0 || n > STORE_CAPACITY * 9 / 10) {
        printf("The number of keys must be from 1 to %d\n", STORE_CAPACITY * 9 / 10);
        return;
    }
    store_init(&bench_store);

    char key[STORE_KEY_MAX];
    for (long i = 0; i < n; ++i) {
        snprintf(key, sizeof key, "key%ld", i);
        store_put(&bench_store, key, "v", NULL);
    }

    long found_hash = 0;
    const char *value = NULL;
    clock_t start = clock();
    for (long i = 0; i < n; ++i) {
        snprintf(key, sizeof key, "key%ld", i);
        if (store_get(&bench_store, key, &value) == STORE_OK) {
            ++found_hash;
        }
    }
    const double t_hash = seconds_since(start);

    long found_linear = 0;
    start = clock();
    for (long i = 0; i < n; ++i) {
        snprintf(key, sizeof key, "key%ld", i);
        if (store_get_linear(&bench_store, key, &value) == STORE_OK) {
            ++found_linear;
        }
    }
    const double t_linear = seconds_since(start);

    printf("Keys: %ld\n", n);
    printf("Via hash: %ld found for %.4f s\n", found_hash, t_hash);
    printf("Brute force: %ld found for %.4f s\n", found_linear, t_linear);
    if (t_hash > 0.0) {
        printf("Hash is faster by %.0f times\n", t_linear / t_hash);
    }
    print_stats(&bench_store);
}

static void demo_collision()
{
    char first[STORE_KEY_MAX];
    char second[STORE_KEY_MAX];

    snprintf(first, sizeof first, "a0");
    const int home = store_home(first);
    second[0] = '\0';
    for (long i = 1; i < 1000000; ++i) {
        snprintf(second, sizeof second, "a%ld", i);
        if (store_home(second) == home) {
            break;
        }
    }
    if (store_home(second) != home) {
        printf("The collision could not be found.\n");
        return;
    }

    store_init(&bench_store);
    store_put(&bench_store, first, "first", NULL);
    store_put(&bench_store, second, "second", NULL);
    printf("Keys %s and %s fall into the same cell %d.\n", first, second, home);

    store_remove(&bench_store, first);
    printf("Deleted %s.\n", first);

    const char *value = NULL;
    if (store_get(&bench_store, second, &value) == STORE_OK) {
        printf("Key %s found: %s\n", second, value);
    } else {
        printf("The %s key is lost: the deleted cell has terminated the sample chain.\n", second);
    }
}

int cli_execute(Session *session, const int argc, char **argv) {
    if (argc == 0) {
        return 1;
    }

    Store *store = session_current(session);
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
        if (store_count(store) == 0) {
            printf("The storage is empty.\n");
        }
        store_for_each(store, print_pair, NULL);
        return 1;
    }

    if (strcmp(argv[0], "sorted") == 0) {
        int depth = 0;
        const int found = range_all(store, print_pair, NULL, &depth);

        printf("Everything is there: %d, sorting recursion depth: %d\n", found, depth);
        return 1;
    }

    if (strcmp(argv[0], "range") == 0) {
        if (argc < 3) {
            fprintf(stderr, "range requires two borders\n");
            return 1;
        }
        if (strcmp(argv[1], argv[2]) > 0) {
            printf("The initial border is larger than the final one: the range is empty.\n");
            return 1;
        }

        int depth = 0;
        const int found = range_query(store, argv[1], argv[2], print_pair, NULL, &depth);
        printf("Found is: %d, sorting recursion depth: %d\n", found, depth);
        return 1;
    }

    if (strcmp(argv[0], "use") == 0) {
        if (argc < 2) {
            fprintf(stderr, "use requires storage number\n");
            return 1;
        }
        const int number = (int) strtol(argv[1], NULL, 10);
        if (!session_select(session, number)) {
            fprintf(stderr, "Storage can be from 1 to %d\n", SESSION_STORES);
            return 1;
        }
        printf("Storage %d (keys: %d) selected\n", number, store_count(session_current(session)));
        return 1;
    }

    if (strcmp(argv[0], "which") == 0) {
        printf("Current storage: %d, keys in it: %d\n",
               session->current + 1, store_count(session_current(session)));
        printf("In another keystore: %d\n", store_count(session_other(session)));
        return 1;
    }

    if (strcmp(argv[0], "copy") == 0) {
        if (argc < 2) {
            fprintf(stderr, "copy requires a key\n");
            return 1;
        }


        const char *value = NULL;
        if (store_get(store, argv[1], &value) != STORE_OK) {
            printf("The key is not in the current storage\n");
            return 1;
        }
        Store *other = session_other(session);
        if (store_put(other, argv[1], value, NULL) != STORE_OK) {
            printf("Couldn't copy\n");
            return 1;
        }
        printf("Copied to another storage\n");
        return 1;
    }

    if (strcmp(argv[0], "sizes") == 0) {
        printf("Storage size: %u bytes\n", (unsigned)sizeof(Store));
        printf("Size of the pointer to it: %u bytes\n", (unsigned)sizeof(Store *));
        printf("You cannot transfer the storage by value: a copy of 2.7 MB\n");
        printf("won't fit on the stack, and the changes in it would have been lost anyway.\n");
        return 1;
    }

    if (strcmp(argv[0], "stats") == 0) {
        print_stats(store);
        return 1;
    }

    if (strcmp(argv[0], "bench") == 0) {
        long n = 10000;
        if (argc >= 2) {
            n = strtol(argv[1], NULL, 10);
        }
        run_bench(n);
        return 1;
    }

    if (strcmp(argv[0], "collide") == 0) {
        demo_collision();
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

static int split_words(char *line, char **argv, const int max_args) {
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

void cli_run_interactive(Session *session) {
    char line[CLI_LINE_MAX];
    char *argv[CLI_ARGS_MAX];
    int running = 1;

    printf("Key-value store. Type 'help' for a list of commands.\n");

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
        running = cli_execute(session, argc, argv);
    }
    printf("The work is completed.\n");
}
