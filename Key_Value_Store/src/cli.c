#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "cli.h"
#include "range.h"
#include "record.h"

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
    printf("  sorted             - all pairs in ascending key order\n");
    printf("  range <from> <to>  - pairs with keys in the range, bounds included\n");
    printf("  stats              - table load and collisions\n");
    printf("  bench [n]          - compare hash and linear search on n keys\n");
    printf("  collide            - delete from the middle of a probe chain\n");
    printf("  record <key> <val> - build a journal record and check it\n");
    printf("  use <1|2>          - switch to the other store\n");
    printf("  which              - which store is selected\n");
    printf("  copy <key>         - copy the pair to the other store\n");
    printf("  sizes              - why the store is passed by address\n");
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

static void print_pair(const char *key, const char *value, void *context) {
    (void)context; /* контекст здесь не нужен, но так устроен обход */
    printf("%s = %s\n", key, value);
}

static void command_list(const Store *store) {
    if (store_count(store) == 0) {
        printf("The store is empty.\n");
        return;
    }
    /* Порядок — порядок ячеек таблицы, то есть произвольный.
       Упорядоченный вывод появится на практике 4. */
    store_for_each(store, print_pair, NULL);
}

static void command_sorted(const Store *store) {
    int depth = 0;
    int found = range_all(store, print_pair, NULL, &depth);
    if (found < 0) {
        printf("Not enough memory for sorting\n");
        return;
    }
    printf("Pairs total: %d, sort recursion depth: %d\n", found, depth);
}

static void command_range(const Store *store, int argc, char *argv[]) {
    if (argc < 3) {
        printf("range needs two bounds\n");
        return;
    }
    if (strcmp(argv[1], argv[2]) > 0) {
        printf("The lower bound is greater than the upper one: the range is empty.\n");
        return;
    }
    int depth = 0;
    int found = range_query(store, argv[1], argv[2], print_pair, NULL, &depth);
    if (found < 0) {
        printf("Not enough memory for sorting\n");
        return;
    }
    printf("Pairs found: %d, sort recursion depth: %d\n", found, depth);
}

static void print_stats(const Store *store) {
    StoreStats stats;
    store_stats(store, &stats);
    printf("Keys: %d of %d (%d%%)\n",
           stats.count, stats.capacity, stats.count * 100 / stats.capacity);
    printf("Cells marked as deleted: %d\n", stats.deleted);
    printf("Keys not in their home cell: %d\n", stats.displaced);
    printf("Longest probe chain: %d\n", stats.longest_probe);
}

static double seconds_since(clock_t start) {
    return (double)(clock() - start) / CLOCKS_PER_SEC;
}

/* Ищет ключи key0 … key(n-1) указанной функцией поиска и возвращает,
   сколько найдено. Время поиска возвращается через elapsed. */
static long search_all(const Store *store, long n,
                       StoreStatus (*get)(const Store *, const char *, const char **),
                       double *elapsed) {
    long found = 0;
    clock_t start = clock();
    for (long i = 0; i < n; ++i) {
        char key[STORE_KEY_MAX];
        snprintf(key, sizeof key, "key%ld", i);
        const char *value = NULL;
        if (get(store, key, &value) == STORE_OK) {
            ++found;
        }
    }
    *elapsed = seconds_since(start);
    return found;
}

/* Хранилище для опытов отдельное, чтобы не засорять пользовательское.
   Оно весит мегабайты, поэтому лежит в куче, а не в стеке, и живёт ровно
   столько, сколько длится опыт. Вызывающий освобождает его через free. */
static Store *new_scratch_store(void) {
    Store *scratch = malloc(sizeof *scratch);
    if (scratch == NULL) {
        printf("Not enough memory\n");
        return NULL;
    }
    store_init(scratch);
    return scratch;
}

/* Кладёт n ключей и ищет каждый двумя способами. */
static void command_bench(int argc, char *argv[]) {
    const long limit = STORE_CAPACITY * 9 / 10;
    long n = argc >= 2 ? strtol(argv[1], NULL, 10) : 10000;
    if (n <= 0 || n > limit) {
        printf("The number of keys must be from 1 to %ld\n", limit);
        return;
    }
    Store *bench = new_scratch_store();
    if (bench == NULL) {
        return;
    }
    for (long i = 0; i < n; ++i) {
        char key[STORE_KEY_MAX];
        snprintf(key, sizeof key, "key%ld", i);
        store_put(bench, key, "v", NULL);
    }

    double t_hash = 0.0;
    long found_hash = search_all(bench, n, store_get, &t_hash);
    double t_linear = 0.0;
    long found_linear = search_all(bench, n, store_get_linear, &t_linear);

    printf("Keys: %ld\n", n);
    printf("Hash:    found %ld in %.4f s\n", found_hash, t_hash);
    printf("Linear:  found %ld in %.4f s\n", found_linear, t_linear);
    if (t_hash > 0.0) {
        printf("Hash is %.0f times faster\n", t_linear / t_hash);
    }
    print_stats(bench);
    free(bench);
}

/* Подбирает два ключа с одной домашней ячейкой, кладёт оба, удаляет первый
   и ищет второй. Если удалённую ячейку пометить пустой, второй не найдётся. */
static void command_collide(void) {
    const char *first = "a0";
    int home = store_home(first);

    /* Перебираем a1, a2, … пока не найдём ключ с той же домашней ячейкой. */
    char second[STORE_KEY_MAX] = "";
    int found = 0;
    for (long i = 1; i < 1000000 && !found; ++i) {
        snprintf(second, sizeof second, "a%ld", i);
        found = store_home(second) == home;
    }
    if (!found) {
        printf("Could not find a collision.\n");
        return;
    }

    Store *scratch = new_scratch_store();
    if (scratch == NULL) {
        return;
    }
    store_put(scratch, first, "first", NULL);
    store_put(scratch, second, "second", NULL);
    printf("Keys %s and %s fall into the same cell %d.\n", first, second, home);

    store_remove(scratch, first);
    printf("Deleted %s.\n", first);

    const char *value = NULL;
    if (store_get(scratch, second, &value) == STORE_OK) {
        printf("Key %s found: %s\n", second, value);
    } else {
        printf("Key %s is lost: the deleted cell broke the probe chain.\n", second);
    }
    free(scratch);
}

/* Печатает байты буфера шестнадцатеричными числами. */
static void print_bytes(const uint8_t *buffer, size_t size) {
    printf("Bytes:");
    for (size_t i = 0; i < size; ++i) {
        printf(" %02X", (unsigned)buffer[i]);
    }
    printf("\n");
}

/* Практика 2: сборка записи журнала, разбор и обнаружение порчи.
   Настоящая запись в файл появится на практике 8. */
static void command_record(int argc, char *argv[]) {
    if (argc < 3) {
        printf("record needs a key and a value\n");
        return;
    }
    const char *key = argv[1];
    const char *value = argv[2];

    uint8_t buffer[512];
    size_t size = 0;
    RecordStatus status = record_build(buffer, sizeof buffer,
                                       record_flags_set(0, RECORD_FLAG_PUT), key, value, &size);
    if (status != RECORD_OK) {
        printf("Build error: %s\n", record_status_text(status));
        return;
    }
    printf("Record size: %zu bytes (header %d + key %zu + value %zu)\n",
           size, RECORD_HEADER_SIZE, strlen(key), strlen(value));
    print_bytes(buffer, size);

    RecordHeader header;
    status = record_parse(buffer, size, &header);
    printf("Parse: %s\n", record_status_text(status));
    record_flags_print(header.flags);
    printf("Key length: %u, value length: %u, checksum: 0x%02X\n",
           (unsigned)header.key_length, (unsigned)header.value_length,
           (unsigned)header.checksum);

    /* Порча байта данных: длина сходится, порчу ловит контрольная сумма.
       Повторное исключающее ИЛИ с той же маской возвращает байт обратно. */
    buffer[size - 1] ^= 0x01u;
    status = record_parse(buffer, size, &header);
    printf("After corrupting the last byte: %s\n", record_status_text(status));
    buffer[size - 1] ^= 0x01u;

    /* Порча длины ключа: её ловит проверка размера, которая стоит раньше,
       потому что по длине мы будем читать файл. */
    buffer[2] ^= 0x01u;
    status = record_parse(buffer, size, &header);
    printf("After corrupting the key length: %s\n", record_status_text(status));
    buffer[2] ^= 0x01u;
}

static void command_use(Session *session, int argc, char *argv[]) {
    if (argc < 2) {
        printf("use needs a store number\n");
        return;
    }
    int number = (int)strtol(argv[1], NULL, 10);
    if (!session_select(session, number)) {
        printf("The store number is from 1 to %d\n", SESSION_STORES);
        return;
    }
    printf("Selected store %d (keys: %d)\n", number, store_count(session_current(session)));
}

static void command_which(Session *session) {
    printf("Current store: %d, keys in it: %d\n",
           session->current + 1, store_count(session_current(session)));
    printf("Keys in the other store: %d\n", store_count(session_other(session)));
}

static void command_copy(Session *session, int argc, char *argv[]) {
    if (argc < 2) {
        printf("copy needs a key\n");
        return;
    }
    const char *value = NULL;
    if (store_get(session_current(session), argv[1], &value) != STORE_OK) {
        printf("The key is not in the current store\n");
        return;
    }
    if (store_put(session_other(session), argv[1], value, NULL) != STORE_OK) {
        printf("Copy failed\n");
        return;
    }
    printf("Copied to the other store\n");
}

static void command_sizes(void) {
    printf("Store size: %u bytes\n", (unsigned)sizeof(Store));
    printf("Pointer size: %u bytes\n", (unsigned)sizeof(Store *));
    printf("The store cannot be passed by value: a 2.7 MB copy\n");
    printf("would not fit on the stack, and changes to it would be lost anyway.\n");
}

int cli_execute(Session *session, int argc, char *argv[]) {
    if (argc == 0) {
        return 1;
    }
    /* Команды над парами работают с текущим хранилищем сессии. */
    Store *store = session_current(session);
    const char *name = argv[0];
    if (strcmp(name, "put") == 0) {
        command_put(store, argc, argv);
    } else if (strcmp(name, "get") == 0) {
        command_get(store, argc, argv);
    } else if (strcmp(name, "del") == 0) {
        command_del(store, argc, argv);
    } else if (strcmp(name, "list") == 0) {
        command_list(store);
    } else if (strcmp(name, "sorted") == 0) {
        command_sorted(store);
    } else if (strcmp(name, "range") == 0) {
        command_range(store, argc, argv);
    } else if (strcmp(name, "stats") == 0) {
        print_stats(store);
    } else if (strcmp(name, "bench") == 0) {
        command_bench(argc, argv);
    } else if (strcmp(name, "collide") == 0) {
        command_collide();
    } else if (strcmp(name, "record") == 0) {
        command_record(argc, argv);
    } else if (strcmp(name, "use") == 0) {
        command_use(session, argc, argv);
    } else if (strcmp(name, "which") == 0) {
        command_which(session);
    } else if (strcmp(name, "copy") == 0) {
        command_copy(session, argc, argv);
    } else if (strcmp(name, "sizes") == 0) {
        command_sizes();
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

void cli_run_interactive(Session *session) {
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
            running = cli_execute(session, argc, argv);
        }
    }
    printf("Done.\n");
}
