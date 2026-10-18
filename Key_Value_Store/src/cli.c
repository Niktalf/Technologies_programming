#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cli.h"
#include "demos.h"
#include "parser.h"
#include "range.h"
#include "script.h"

#define CLI_LINE_MAX 256

/* Практика 8: консольная программа — клиент библиотеки. Все операции
   с данными идут через публичный интерфейс kvstore.h, поэтому каждая
   из них сразу дописывается в журнал и переживает перезапуск.

   Все ответы на команды, включая сообщения об ошибках, идут в обычный вывод:
   они часть диалога и должны стоять в нём по порядку. */

void cli_print_help(void) {
    printf("Available commands:\n");
    printf("  put <key> <value>    - store a pair\n");
    printf("      a value with spaces goes in quotes: put name \"two words\"\n");
    printf("      a quote inside a value is written as \\\"\n");
    printf("  get <key>            - get the value\n");
    printf("  del <key>            - delete the pair\n");
    printf("  list                 - list all pairs\n");
    printf("  sorted               - all pairs in ascending key order\n");
    printf("  range <from> <to>    - pairs with keys in the range, bounds included\n");
    printf("  stats                - keys and journal records\n");
    printf("  use <1|2>            - switch to the other store\n");
    printf("  which                - which store is selected and where its journal is\n");
    printf("  copy <key>           - copy the pair to the other store\n");
    printf("  open <file>          - open another journal in place of the current one\n");
    printf("  close                - close the current store, the journal stays on disk\n");
    printf("  sizes                - why the store is reached only by address\n");
    printf("  script <file> [stop] - run commands from a file\n");
    printf("  genscript <file> <n> - write a script of n put commands\n");
    printf("  stress <n> <script>  - n times: open a database, load a script, close\n");
    printf("  bench [n]            - compare hash and linear search on n keys\n");
    printf("  collide              - delete from the middle of a probe chain\n");
    printf("  record <key> <val>   - build a journal record and check it\n");
    printf("  help                 - this list\n");
    printf("  quit                 - exit\n");
}

/* Печатает ошибку библиотеки. «Ключ не найден» — ответ, а не ошибка:
   скрипт из-за него не прерывается. */
static int report(KvStatus status) {
    printf("Error: %s\n", kv_status_text(status));
    return status == KV_ERR_NOT_FOUND ? CLI_OK : CLI_ERROR;
}

static void print_pair(const char *key, const char *value, void *context) {
    (void)context; /* контекст здесь не нужен, но так устроен обход */
    printf("%s = %s\n", key, value);
}

static int command_put(KvStore *store, int argc, char *argv[]) {
    if (argc < 3) {
        printf("put needs a key and a value\n");
        return CLI_ERROR;
    }
    KvStatus status = kv_put(store, argv[1], argv[2]);
    if (status != KV_OK) {
        return report(status);
    }
    printf("saved\n");
    return CLI_OK;
}

static int command_get(const KvStore *store, int argc, char *argv[]) {
    if (argc < 2) {
        printf("get needs a key\n");
        return CLI_ERROR;
    }
    const char *value = NULL;
    KvStatus status = kv_get(store, argv[1], &value);
    if (status != KV_OK) {
        return report(status);
    }
    printf("%s\n", value);
    return CLI_OK;
}

static int command_del(KvStore *store, int argc, char *argv[]) {
    if (argc < 2) {
        printf("del needs a key\n");
        return CLI_ERROR;
    }
    KvStatus status = kv_delete(store, argv[1]);
    if (status != KV_OK) {
        return report(status);
    }
    printf("deleted\n");
    return CLI_OK;
}

static int command_list(const KvStore *store) {
    if (kv_count(store) == 0) {
        printf("The store is empty.\n");
        return CLI_OK;
    }
    /* Порядок — порядок ячеек таблицы, то есть произвольный.
       Упорядоченный вывод — команда sorted. */
    kv_for_each(store, print_pair, NULL);
    return CLI_OK;
}

static int command_sorted(const KvStore *store) {
    int depth = 0;
    int found = range_all(store, print_pair, NULL, &depth);
    if (found < 0) {
        printf("Not enough memory for sorting\n");
        return CLI_ERROR;
    }
    printf("Pairs total: %d, sort recursion depth: %d\n", found, depth);
    return CLI_OK;
}

static int command_range(const KvStore *store, int argc, char *argv[]) {
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

static int command_stats(const KvStore *store) {
    printf("Keys: %d\n", kv_count(store));
    printf("Journal records: %ld, obsolete among them: %ld\n",
           kv_journal_records(store), kv_journal_garbage(store));
    printf("Discarded as damaged on open: %ld\n", kv_recovered_broken(store));
    return CLI_OK;
}

static int command_copy(Session *session, KvStore *store, int argc, char *argv[]) {
    if (argc < 2) {
        printf("copy needs a key\n");
        return CLI_ERROR;
    }
    KvStore *other = session_other(session);
    if (other == NULL) {
        printf("The other store is closed\n");
        return CLI_ERROR;
    }
    const char *value = NULL;
    KvStatus status = kv_get(store, argv[1], &value);
    if (status == KV_OK) {
        status = kv_put(other, argv[1], value);
    }
    if (status != KV_OK) {
        return report(status);
    }
    printf("Copied to the other store\n");
    return CLI_OK;
}

static int command_close(Session *session, KvStore *store) {
    int count = kv_count(store);
    session_close(session);
    printf("Store %d closed, pairs freed: %d. Journal %s stays on disk.\n",
           session->current + 1, count, session_current_path(session));
    return CLI_OK;
}

static int command_use(Session *session, int argc, char *argv[]) {
    if (argc < 2) {
        printf("use needs a store number\n");
        return CLI_ERROR;
    }
    if (session_select(session, (int)strtol(argv[1], NULL, 10)) != KV_OK) {
        printf("The store number is from 1 to %d\n", SESSION_STORES);
        return CLI_ERROR;
    }
    printf("Selected store %d, journal %s, %s\n", session->current + 1,
           session_current_path(session), session_current(session) != NULL ? "open" : "closed");
    return CLI_OK;
}

static int command_which(Session *session) {
    KvStore *store = session_current(session);
    printf("Current store: %d, journal %s, ", session->current + 1,
           session_current_path(session));
    if (store != NULL) {
        printf("keys: %d\n", kv_count(store));
    } else {
        printf("closed\n");
    }
    return CLI_OK;
}

static int command_open(Session *session, int argc, char *argv[]) {
    if (argc < 2) {
        printf("open needs a journal file name\n");
        return CLI_ERROR;
    }
    KvStatus status = session_open(session, argv[1]);
    if (status != KV_OK) {
        return report(status);
    }
    KvStore *store = session_current(session);
    printf("Journal opened %s: keys %d, damaged records discarded %ld\n",
           argv[1], kv_count(store), kv_recovered_broken(store));
    return CLI_OK;
}

/* Практика 5 показывала, почему хранилище передаётся по адресу.
   Практика 8: клиент библиотеки не знает даже размера KvStore —
   в публичном заголовке только имя типа. Передать его по значению
   или скопировать просто не получится: компилятор не знает, сколько
   байт копировать. Остаётся только указатель. */
static int command_sizes(void) {
    printf("Session size: %u bytes, pointer to a store: %u bytes\n",
           (unsigned)sizeof(Session), (unsigned)sizeof(KvStore *));
    printf("The size of KvStore itself is unknown here: the public header\n");
    printf("declares only its name. A client cannot copy the store or pass it\n");
    printf("by value at all - only a pointer, and only library functions see inside.\n");
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

/* Заливает в базу все команды put из скрипта, ничего не печатая:
   при ста повторах вывод каждой команды занял бы миллион строк.
   Возвращает KV_OK или первую ошибку; число пар — через added. */
static KvStatus fill_from_script(KvStore *store, const char *path, long *added) {
    *added = 0;
    FILE *file = fopen(path, "r");
    if (file == NULL) {
        return KV_ERR_OPEN;
    }
    KvStatus status = KV_OK;
    char line[CLI_LINE_MAX];
    while (status == KV_OK && fgets(line, (int)sizeof line, file) != NULL) {
        char *argv[PARSER_ARGS_MAX];
        int argc = 0;
        int parsed = parser_split(line, argv, PARSER_ARGS_MAX, &argc) == PARSER_OK;
        if (parsed && argc >= 3 && strcmp(argv[0], "put") == 0) {
            status = kv_put(store, argv[1], argv[2]);
            if (status == KV_OK) {
                ++*added;
            }
        }
    }
    fclose(file); /* файл открыт на чтение: потерять при закрытии нечего */
    return status;
}

/* n раз: открыть базу во временном файле, залить скрипт, закрыть, удалить
   файл. Если что-то не освобождается, санитайзер назовёт место утечки. */
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
    const char *path = "stress.tmp.db";
    long added = 0;
    KvStatus status = KV_OK;
    long done = 0;
    while (done < times && status == KV_OK) {
        remove(path); /* каждый повтор начинает с пустой базы */
        KvStore *store = NULL;
        status = kv_open(path, &store);
        if (status == KV_OK) {
            status = fill_from_script(store, argv[2], &added);
        }
        kv_close(store); /* закрыть NULL безопасно */
        ++done;
    }
    remove(path);
    if (status != KV_OK) {
        printf("Repeat %ld: %s\n", done, kv_status_text(status));
        return CLI_ERROR;
    }
    printf("Databases opened, filled and closed: %ld, pairs in each: %ld\n", times, added);
    return CLI_OK;
}

/* Команды, которым нужно открытое хранилище. */
static int execute_on_store(Session *session, KvStore *store, int argc, char *argv[]) {
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
        return command_stats(store);
    }
    if (strcmp(name, "copy") == 0) {
        return command_copy(session, store, argc, argv);
    }
    if (strcmp(name, "close") == 0) {
        return command_close(session, store);
    }
    printf("Unknown command: %s\n", name);
    printf("Type help for the list of commands.\n");
    return CLI_ERROR;
}

int cli_execute(Session *session, int argc, char *argv[]) {
    if (session == NULL || argc == 0) {
        return CLI_OK;
    }
    /* Сначала команды, которым открытое хранилище не нужно. */
    const char *name = argv[0];
    if (strcmp(name, "quit") == 0 || strcmp(name, "exit") == 0) {
        return CLI_QUIT;
    }
    if (strcmp(name, "help") == 0) {
        cli_print_help();
        return CLI_OK;
    }
    if (strcmp(name, "use") == 0) {
        return command_use(session, argc, argv);
    }
    if (strcmp(name, "which") == 0) {
        return command_which(session);
    }
    if (strcmp(name, "open") == 0) {
        return command_open(session, argc, argv);
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
    if (strcmp(name, "bench") == 0) {
        return demo_bench(argc >= 2 ? strtol(argv[1], NULL, 10) : 10000);
    }
    if (strcmp(name, "collide") == 0) {
        return demo_collision();
    }
    if (strcmp(name, "record") == 0) {
        if (argc < 3) {
            printf("record needs a key and a value\n");
            return CLI_ERROR;
        }
        return demo_record(argv[1], argv[2]);
    }

    /* Всё остальное работает с данными и требует открытого хранилища. */
    KvStore *store = session_current(session);
    if (store == NULL) {
        printf("Store %d is closed. Open it: open <file>\n", session->current + 1);
        return CLI_ERROR;
    }
    return execute_on_store(session, store, argc, argv);
}

/* Дочитывает и выбрасывает остаток строки до перевода строки или конца ввода. */
static void skip_rest_of_line(void) {
    int c = getchar();
    while (c != '\n' && c != EOF) {
        c = getchar();
    }
}

void cli_run_interactive(Session *session) {
    printf("Key-value store. Journals: %s and %s.\n", session->path[0], session->path[1]);
    printf("Type help for the list of commands.\n");

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
