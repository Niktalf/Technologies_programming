#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "cli.h"
#include "parser.h"
#include "range.h"
#include "record.h"
#include "script.h"

#define CLI_LINE_MAX 256

/* Все ответы на команды, включая сообщения об ошибках, идут в обычный вывод:
   они часть диалога и должны стоять в нём по порядку, сразу после приглашения.
   При выполнении скрипта это особенно важно: у двух потоков разная
   буферизация, и сообщения перемешались бы не в том порядке.

   Каждая команда возвращает CLI_OK или CLI_ERROR. Ошибка — это команда,
   которая не выполнилась: нет аргумента, неверный аргумент, не хватило
   памяти. «Ключ не найден» — это ответ, а не ошибка. */

void cli_print_help(void) {
    printf("Available commands:\n");
    printf("  put <key> <value>  - store a pair\n");
    printf("      a value with spaces goes in quotes: put name \"two words\"\n");
    printf("      a quote inside a value is written as \\\"\n");
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
    printf("  script <file> [stop] - run commands from a file\n");
    printf("  genscript <file> <n> - write a script of n put commands\n");
    printf("  stress <n> <script>  - n times: create a store, load a script, close\n");
    printf("  close              - close the current store and open an empty one\n");
    printf("  help               - this list\n");
    printf("  quit               - exit\n");
}

static void report(StoreStatus status) {
    printf("Error: %s\n", store_status_text(status));
}

static int command_put(Store *store, int argc, char *argv[]) {
    if (argc < 3) {
        printf("put needs a key and a value\n");
        return CLI_ERROR;
    }
    int was_present = 0;
    StoreStatus status = store_put(store, argv[1], argv[2], &was_present);
    if (status != STORE_OK) {
        report(status);
        return CLI_ERROR;
    }
    printf(was_present ? "updated\n" : "added\n");
    return CLI_OK;
}

static int command_get(const Store *store, int argc, char *argv[]) {
    if (argc < 2) {
        printf("get needs a key\n");
        return CLI_ERROR;
    }
    const char *value = NULL;
    StoreStatus status = store_get(store, argv[1], &value);
    if (status != STORE_OK) {
        report(status);
        return status == STORE_ERR_NOT_FOUND ? CLI_OK : CLI_ERROR;
    }
    printf("%s\n", value);
    return CLI_OK;
}

static int command_del(Store *store, int argc, char *argv[]) {
    if (argc < 2) {
        printf("del needs a key\n");
        return CLI_ERROR;
    }
    StoreStatus status = store_remove(store, argv[1]);
    if (status != STORE_OK) {
        report(status);
        return status == STORE_ERR_NOT_FOUND ? CLI_OK : CLI_ERROR;
    }
    printf("deleted\n");
    return CLI_OK;
}

static void print_pair(const char *key, const char *value, void *context) {
    (void)context; /* контекст здесь не нужен, но так устроен обход */
    printf("%s = %s\n", key, value);
}

static int command_list(const Store *store) {
    if (store_count(store) == 0) {
        printf("The store is empty.\n");
        return CLI_OK;
    }
    /* Порядок — порядок ячеек таблицы, то есть произвольный.
       Упорядоченный вывод — команда sorted. */
    store_for_each(store, print_pair, NULL);
    return CLI_OK;
}

static int command_sorted(const Store *store) {
    int depth = 0;
    int found = range_all(store, print_pair, NULL, &depth);
    if (found < 0) {
        printf("Not enough memory for sorting\n");
        return CLI_ERROR;
    }
    printf("Pairs total: %d, sort recursion depth: %d\n", found, depth);
    return CLI_OK;
}

static int command_range(const Store *store, int argc, char *argv[]) {
    if (argc < 3) {
        printf("range needs two bounds\n");
        return CLI_ERROR;
    }
    if (strcmp(argv[1], argv[2]) > 0) {
        printf("The lower bound is greater than the upper one: the range is empty.\n");
        return CLI_OK;
    }
    int depth = 0;
    int found = range_query(store, argv[1], argv[2], print_pair, NULL, &depth);
    if (found < 0) {
        printf("Not enough memory for sorting\n");
        return CLI_ERROR;
    }
    printf("Pairs found: %d, sort recursion depth: %d\n", found, depth);
    return CLI_OK;
}

static void print_stats(const Store *store) {
    StoreStats stats;
    store_stats(store, &stats);
    printf("Keys: %d of %d (%d%%)\n",
           stats.count, stats.capacity, stats.count * 100 / stats.capacity);
    printf("Cells marked as deleted: %d\n", stats.deleted);
    printf("Keys not in their home cell: %d\n", stats.displaced);
    printf("Longest probe chain: %d\n", stats.longest_probe);
    printf("Table rebuilds: %ld\n", stats.rehash_count);
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
        char key[32];
        snprintf(key, sizeof key, "key%ld", i);
        const char *value = NULL;
        if (get(store, key, &value) == STORE_OK) {
            ++found;
        }
    }
    *elapsed = seconds_since(start);
    return found;
}

/* Кладёт n ключей и ищет каждый двумя способами. Хранилище для замеров
   отдельное, чтобы не засорять пользовательское. Сама структура маленькая —
   таблица в куче, — поэтому она локальная; таблица растёт сама. */
static int command_bench(int argc, char *argv[]) {
    long n = argc >= 2 ? strtol(argv[1], NULL, 10) : 10000;
    if (n <= 0) {
        printf("The number of keys must be positive\n");
        return CLI_ERROR;
    }
    Store bench;
    if (!store_init(&bench)) {
        printf("Not enough memory\n");
        return CLI_ERROR;
    }
    for (long i = 0; i < n; ++i) {
        char key[32];
        snprintf(key, sizeof key, "key%ld", i);
        store_put(&bench, key, "v", NULL);
    }

    double t_hash = 0.0;
    long found_hash = search_all(&bench, n, store_get, &t_hash);
    double t_linear = 0.0;
    long found_linear = search_all(&bench, n, store_get_linear, &t_linear);

    printf("Keys: %ld\n", n);
    printf("Hash:    found %ld in %.4f s\n", found_hash, t_hash);
    printf("Linear:  found %ld in %.4f s\n", found_linear, t_linear);
    if (t_hash > 0.0) {
        printf("Hash is %.0f times faster\n", t_linear / t_hash);
    }
    print_stats(&bench);
    store_destroy(&bench);
    return CLI_OK;
}

/* Подбирает два ключа с одной домашней ячейкой, кладёт оба, удаляет первый
   и ищет второй. Если удалённую ячейку пометить пустой, второй не найдётся. */
static int command_collide(void) {
    /* Ключи подбираются уже в том хранилище, где будут лежать:
       номер домашней ячейки зависит от размера таблицы. */
    Store scratch;
    if (!store_init(&scratch)) {
        printf("Not enough memory\n");
        return CLI_ERROR;
    }
    const char *first = "a0";
    int home = store_home(&scratch, first);

    /* Перебираем a1, a2, … пока не найдём ключ с той же домашней ячейкой. */
    char second[32] = "";
    int found = 0;
    for (long i = 1; i < 1000000 && !found; ++i) {
        snprintf(second, sizeof second, "a%ld", i);
        found = store_home(&scratch, second) == home;
    }
    if (!found) {
        printf("Could not find a collision.\n");
        store_destroy(&scratch);
        return CLI_OK;
    }

    store_put(&scratch, first, "first", NULL);
    store_put(&scratch, second, "second", NULL);
    printf("Keys %s and %s fall into the same cell %d.\n", first, second, home);

    store_remove(&scratch, first);
    printf("Deleted %s.\n", first);

    const char *value = NULL;
    if (store_get(&scratch, second, &value) == STORE_OK) {
        printf("Key %s found: %s\n", second, value);
    } else {
        printf("Key %s is lost: the deleted cell broke the probe chain.\n", second);
    }
    store_destroy(&scratch);
    return CLI_OK;
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
static int command_record(int argc, char *argv[]) {
    if (argc < 3) {
        printf("record needs a key and a value\n");
        return CLI_ERROR;
    }
    const char *key = argv[1];
    const char *value = argv[2];

    uint8_t buffer[512];
    size_t size = 0;
    RecordStatus status = record_build(buffer, sizeof buffer,
                                       record_flags_set(0, RECORD_FLAG_PUT), key, value, &size);
    if (status != RECORD_OK) {
        printf("Build error: %s\n", record_status_text(status));
        return CLI_ERROR;
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
    return CLI_OK;
}

static int command_use(Session *session, int argc, char *argv[]) {
    if (argc < 2) {
        printf("use needs a store number\n");
        return CLI_ERROR;
    }
    int number = (int)strtol(argv[1], NULL, 10);
    if (!session_select(session, number)) {
        printf("The store number is from 1 to %d\n", SESSION_STORES);
        return CLI_ERROR;
    }
    printf("Selected store %d (keys: %d)\n", number, store_count(session_current(session)));
    return CLI_OK;
}

static int command_which(Session *session) {
    printf("Current store: %d, keys in it: %d\n",
           session->current + 1, store_count(session_current(session)));
    printf("Keys in the other store: %d\n", store_count(session_other(session)));
    return CLI_OK;
}

static int command_copy(Session *session, int argc, char *argv[]) {
    if (argc < 2) {
        printf("copy needs a key\n");
        return CLI_ERROR;
    }
    const char *value = NULL;
    if (store_get(session_current(session), argv[1], &value) != STORE_OK) {
        printf("The key is not in the current store\n");
        return CLI_OK;
    }
    if (store_put(session_other(session), argv[1], value, NULL) != STORE_OK) {
        printf("Copy failed\n");
        return CLI_ERROR;
    }
    printf("Copied to the other store\n");
    return CLI_OK;
}

static int command_sizes(void) {
    printf("Store size: %u bytes\n", (unsigned)sizeof(Store));
    printf("Pointer size: %u bytes\n", (unsigned)sizeof(Store *));
    printf("The structure itself is small now: the table, keys and values are on the heap.\n");
    printf("But it still cannot be passed by value: the copy would share\n");
    printf("one table with the original, and changes to one would break the other.\n");
    return CLI_OK;
}

/* Пишет скрипт из n команд put. Руками файл на тысячу строк не набрать,
   а проверять разбор скриптов на нём нужно. */
static int command_genscript(int argc, char *argv[]) {
    if (argc < 3) {
        printf("genscript needs a file name and a number of pairs\n");
        return CLI_ERROR;
    }
    const char *path = argv[1];
    long n = strtol(argv[2], NULL, 10);
    if (n <= 0 || n > 10000000) {
        printf("The number of pairs must be from 1 to 10000000\n");
        return CLI_ERROR;
    }
    FILE *file = fopen(path, "w");
    if (file == NULL) {
        printf("Cannot create the file: %s\n", path);
        return CLI_ERROR;
    }
    fprintf(file, "# script of %ld pairs, made by genscript\n", n);
    for (long i = 1; i <= n; ++i) {
        fprintf(file, "put key%06ld \"value number %ld\"\n", i, i);
    }
    /* Ошибка записи (например, кончилось место на диске) видна только
       при закрытии файла, поэтому результат fclose тоже проверяется. */
    if (fclose(file) != 0) {
        printf("Error writing the file: %s\n", path);
        return CLI_ERROR;
    }
    printf("Script written %s: %ld commands\n", path, n);
    return CLI_OK;
}

static int command_script(Session *session, int argc, char *argv[]) {
    if (argc < 2) {
        printf("script needs a file name\n");
        return CLI_ERROR;
    }
    int stop = argc >= 3 && strcmp(argv[2], "stop") == 0;
    /* Ошибки внутри скрипта уже сообщены с номерами строк; сама команда
       script выполнилась, даже если скрипт был с ошибками. */
    script_run(session, argv[1], stop);
    return CLI_OK;
}

/* Заливает в хранилище все команды put из скрипта, ничего не печатая:
   при ста повторах вывод каждой команды занял бы миллион строк.
   Возвращает число залитых пар или -1, если файл не открылся. */
static long fill_from_script(Store *store, const char *path) {
    FILE *file = fopen(path, "r");
    if (file == NULL) {
        return -1;
    }
    long added = 0;
    char line[CLI_LINE_MAX];
    while (fgets(line, (int)sizeof line, file) != NULL) {
        char *argv[PARSER_ARGS_MAX];
        int argc = 0;
        int parsed = parser_split(line, argv, PARSER_ARGS_MAX, &argc) == PARSER_OK;
        if (parsed && argc >= 3 && strcmp(argv[0], "put") == 0
            && store_put(store, argv[1], argv[2], NULL) == STORE_OK) {
            ++added;
        }
    }
    fclose(file);
    return added;
}

/* Много раз подряд: создать хранилище, залить в него скрипт, закрыть.
   Если хоть что-то не освобождается, потребление памяти растёт с каждым
   повтором, а санитайзер в конце назовёт место утечки. */
static int command_stress(int argc, char *argv[]) {
    if (argc < 3) {
        printf("stress needs a repeat count and a script: stress 100 pairs1000.kv\n");
        return CLI_ERROR;
    }
    long times = strtol(argv[1], NULL, 10);
    if (times <= 0) {
        printf("The repeat count must be positive\n");
        return CLI_ERROR;
    }
    const char *script = argv[2];
    long done = 0;
    long pairs = 0;
    int failed = 0;
    for (long i = 0; i < times && !failed; ++i) {
        Store temp;
        if (!store_init(&temp)) {
            printf("Not enough memory\n");
            failed = 1;
        } else {
            pairs = fill_from_script(&temp, script);
            if (pairs < 0) {
                printf("Cannot open the script: %s\n", script);
                failed = 1;
            } else {
                ++done;
            }
            store_destroy(&temp);
            store_destroy(&temp); /* повторное освобождение безопасно: указатели обнулены */
        }
    }
    printf("Stores created, filled and closed: %ld of %ld\n", done, times);
    if (done > 0) {
        printf("Pairs in each: %ld\n", pairs);
    }
    return failed ? CLI_ERROR : CLI_OK;
}

/* Закрытие освобождает всё: ключи, значения, саму таблицу. Чтобы хранилищем
   можно было пользоваться дальше, сразу создаётся новое, пустое. */
static int command_close(Session *session) {
    Store *store = session_current(session);
    int count = store_count(store);
    store_destroy(store);
    if (!store_init(store)) {
        printf("Not enough memory for a new store\n");
        return CLI_ERROR;
    }
    printf("Store %d closed, pairs freed: %d. A new empty one is open.\n",
           session->current + 1, count);
    return CLI_OK;
}

int cli_execute(Session *session, int argc, char *argv[]) {
    if (argc == 0) {
        return CLI_OK;
    }
    /* Команды над парами работают с текущим хранилищем сессии. */
    Store *store = session_current(session);
    const char *name = argv[0];
    if (strcmp(name, "put") == 0) {
        return command_put(store, argc, argv);
    }
    if (strcmp(name, "get") == 0) {
        return command_get(store, argc, argv);
    }
    if (strcmp(name, "del") == 0) {
        return command_del(store, argc, argv);
    }
    if (strcmp(name, "list") == 0) {
        return command_list(store);
    }
    if (strcmp(name, "sorted") == 0) {
        return command_sorted(store);
    }
    if (strcmp(name, "range") == 0) {
        return command_range(store, argc, argv);
    }
    if (strcmp(name, "stats") == 0) {
        print_stats(store);
        return CLI_OK;
    }
    if (strcmp(name, "bench") == 0) {
        return command_bench(argc, argv);
    }
    if (strcmp(name, "collide") == 0) {
        return command_collide();
    }
    if (strcmp(name, "record") == 0) {
        return command_record(argc, argv);
    }
    if (strcmp(name, "use") == 0) {
        return command_use(session, argc, argv);
    }
    if (strcmp(name, "which") == 0) {
        return command_which(session);
    }
    if (strcmp(name, "copy") == 0) {
        return command_copy(session, argc, argv);
    }
    if (strcmp(name, "sizes") == 0) {
        return command_sizes();
    }
    if (strcmp(name, "script") == 0) {
        return command_script(session, argc, argv);
    }
    if (strcmp(name, "genscript") == 0) {
        return command_genscript(argc, argv);
    }
    if (strcmp(name, "stress") == 0) {
        return command_stress(argc, argv);
    }
    if (strcmp(name, "close") == 0) {
        return command_close(session);
    }
    if (strcmp(name, "help") == 0) {
        cli_print_help();
        return CLI_OK;
    }
    if (strcmp(name, "quit") == 0 || strcmp(name, "exit") == 0) {
        return CLI_QUIT;
    }
    printf("Unknown command: %s\n", name);
    printf("Type help for the list of commands.\n");
    return CLI_ERROR;
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
            char *argv[PARSER_ARGS_MAX];
            int argc = 0;
            ParserStatus status = parser_split(line, argv, PARSER_ARGS_MAX, &argc);
            if (status != PARSER_OK) {
                printf("Cannot parse the line: %s\n", parser_status_text(status));
            } else {
                running = cli_execute(session, argc, argv) != CLI_QUIT;
            }
        }
    }
    printf("Done.\n");
}
