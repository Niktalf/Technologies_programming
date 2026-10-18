#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "binlog.h"
#include "generator.h"
#include "level.h"
#include "query.h"
#include "shell.h"
#include "sort.h"
#include "view.h"
#include "stats.h"
#include "timestamp.h"

#define SHELL_LINE_MAX  256
#define SHELL_WORDS_MAX 8

static void print_help(void) {
    printf("Available commands:\n");
    printf("  load <file>                - load a log\n");
    printf("  bsave <file>               - save the parsed log in binary form\n");
    printf("  bload <file>               - read a binary log\n");
    printf("  gen <file> <count> [seed]  - create a test log\n");
    printf("  count                      - number of records and lines\n");
    printf("  head                       - first ten records\n");
    printf("  tail                       - last ten records\n");
    printf("  stats                      - records by level\n");
    printf("  hours                      - distribution by hour\n");
    printf("  modules                    - records by module\n");
    printf("  order time|level|module    - viewing order (records do not move)\n");
    printf("  view [n]                   - show n records in the chosen order\n");
    printf("  span                       - the earliest and the latest time\n");
    printf("  cost                       - compare sorting pointers and records\n");
    printf("  sort                       - sort the records themselves by time\n");
    printf("  grep <word>                - records whose text contains the word\n");
    printf("  module <name>              - records of one module\n");
    printf("  between <date> <time> <date> <time> - records in the interval\n");
    printf("  since <date> <time>        - records from the moment on,\n");
    printf("                               e.g. since 2026-03-15 12:00:00\n");
    printf("  level <levels...>          - set the filter, e.g. level error fatal\n");
    printf("  level all | none           - enable all levels or clear them all\n");
    printf("  filter                     - show the current filter\n");
    printf("  time                       - check the time packing\n");
    printf("  stress [n]                 - load and free the log n times\n");
    printf("  help                       - this list\n");
    printf("  quit                       - exit\n");
}

/* Дочитывает и выбрасывает остаток строки до перевода строки или конца ввода. */
static void skip_rest_of_line(void) {
    int c = getchar();
    while (c != '\n' && c != EOF) {
        c = getchar();
    }
}

/* Разбивает строку на слова по пробелам. Пробелы заменяются нулями,
   в words попадают указатели внутрь строки. К нижнему регистру приводится
   только имя команды: остальные слова могут быть именами файлов.
   Кавычки появятся на практике 6. */
static int split_words(char *line, char *words[], int max_words) {
    int count = 0;
    char *p = line;
    while (count < max_words) {
        while (*p != '\0' && isspace((unsigned char)*p)) {
            *p = '\0';
            ++p;
        }
        if (*p == '\0') {
            return count;
        }
        words[count] = p;
        while (*p != '\0' && !isspace((unsigned char)*p)) {
            if (count == 0) {
                *p = (char)tolower((unsigned char)*p);
            }
            ++p;
        }
        ++count;
    }
    return count;
}

/* Читает строку и разбивает её на слова. Возвращает число слов
   либо -1, если ввод закончился. */
static int read_words(char *buffer, size_t size, char *words[], int max_words) {
    if (fgets(buffer, (int)size, stdin) == NULL) {
        return -1;
    }
    if (strchr(buffer, '\n') == NULL) {
        skip_rest_of_line();
    }
    return split_words(buffer, words, max_words);
}

/* Демонстрация упаковки времени: порядок чисел совпадает с порядком моментов. */
static void demo_time(void) {
    static const DateTime SAMPLES[] = {
        { 2026, 3, 15, 23, 59, 59 },
        { 2026, 3, 16, 0, 0, 0 },
        { 2026, 12, 31, 23, 59, 59 },
        { 2027, 1, 1, 0, 0, 0 }
    };
    const size_t count = sizeof SAMPLES / sizeof SAMPLES[0];

    Timestamp previous = TIMESTAMP_INVALID;
    for (size_t i = 0; i < count; ++i) {
        const DateTime *m = &SAMPLES[i];
        Timestamp value = timestamp_pack(m);
        printf("%04d-%02d-%02d %02d:%02d:%02d -> %lld -> ",
               m->year, m->month, m->day, m->hour, m->minute, m->second, (long long)value);
        timestamp_print(value);
        printf("  hour %d\n", timestamp_hour(value));
        if (previous != TIMESTAMP_INVALID && value <= previous) {
            printf("  ERROR: order is broken\n");
        }
        previous = value;
    }
    DateTime invalid = { 2026, 13, 1, 0, 0, 0 };
    printf("Invalid date 2026-13-01: %lld (expected %lld)\n",
           (long long)timestamp_pack(&invalid), (long long)TIMESTAMP_INVALID);
}

/* Разбор аргументов команды level. Возвращает новую маску. */
static LevelMask parse_level_arguments(LevelMask mask, char *words[], int count) {
    if (count < 2) {
        printf("level needs at least one level, all or none\n");
        return mask;
    }
    if (strcmp(words[1], "all") == 0) {
        return LEVEL_MASK_ALL;
    }
    if (strcmp(words[1], "none") == 0) {
        return LEVEL_MASK_NONE;
    }

    LevelMask result = LEVEL_MASK_NONE;
    for (int i = 1; i < count; ++i) {
        Level level = level_from_word(words[i]);
        if (level == LEVEL_UNKNOWN) {
            printf("Unknown level: %s\n", words[i]);
        } else {
            result = level_mask_set(result, level);
        }
    }
    /* Если ни одно слово не оказалось уровнем, прежний фильтр сохраняется:
       опечатка не должна молча скрыть весь журнал. */
    if (result == LEVEL_MASK_NONE) {
        printf("Filter unchanged.\n");
        return mask;
    }
    return result;
}

static void print_load_report(const LogFile *log) {
    printf("File: %s\n", log->path);
    printf("Lines: %ld, records: %ld\n", log->lines, log->count);
    if (log->broken > 0) {
        printf("Lines that could not be parsed: %ld\n", log->broken);
    }
    if (log->no_memory > 0) {
        printf("Records lost for lack of memory: %ld\n", log->no_memory);
    }
    printf("Array capacity: %ld records\n", log->capacity);
    if (log->truncated > 0) {
        printf("Lines longer than the buffer: %ld\n", log->truncated);
    }
}

/* Проверяет, что журнал загружен, и печатает подсказку, если нет. */
static int require_log(const LogFile *log) {
    if (!log->loaded) {
        printf("Log is not loaded. Use load <file> or gen.\n");
        return 0;
    }
    return 1;
}

static void command_load(LogFile *log, char *words[], int count) {
    if (count < 2) {
        printf("Specify the file: load <file>\n");
        return;
    }
    clock_t start = clock();
    LogStatus status = logfile_load(log, words[1]);
    if (status != LOG_OK) {
        printf("Error: %s (%s)\n", status_text(status), words[1]);
        return;
    }
    printf("Parsed in %.3f s\n", (double)(clock() - start) / CLOCKS_PER_SEC);
    view_reset(); /* указатели смотрели на записи прежнего файла */
    print_load_report(log);
}

static void command_gen(char *words[], int count) {
    if (count < 3) {
        printf("Usage: gen <file> <line count> [seed]\n");
        return;
    }
    char *end = NULL;
    long lines = strtol(words[2], &end, 10);
    if (*end != '\0' || lines <= 0) {
        printf("The line count must be a positive number\n");
        return;
    }
    unsigned long seed = 1;
    if (count >= 4) {
        seed = strtoul(words[3], &end, 10);
        if (*end != '\0') {
            printf("The seed must be a number\n");
            return;
        }
    }
    LogStatus status = generator_write(words[1], lines, (unsigned int)seed);
    if (status != LOG_OK) {
        printf("Cannot create %s: %s\n", words[1], status_text(status));
        return;
    }
    printf("Created %s: %ld lines, seed %lu\n", words[1], lines, seed);
}

static void command_count(const LogFile *log, LevelMask mask) {
    if (!require_log(log)) {
        return;
    }
    printf("Records: %ld, lines in the file: %ld, passing the level filter: %ld\n",
           log->count, log->lines, logfile_count_matching(log, mask));
}

/* Какую часть статистики печатать. */
typedef enum {
    REPORT_LEVELS,
    REPORT_HOURS,
    REPORT_MODULES
} Report;

static void command_stats(const LogFile *log, Report report) {
    if (!require_log(log)) {
        return;
    }
    /* Статистика весит несколько килобайт и нужна только на время команды. */
    Stats stats;
    stats_collect(log, &stats);
    if (report == REPORT_LEVELS) {
        stats_print_levels(&stats, log->count);
    } else if (report == REPORT_HOURS) {
        stats_print_hours(&stats);
    } else {
        stats_print_modules(&stats);
    }
}

static void command_sort(LogFile *log) {
    if (!require_log(log)) {
        return;
    }
    int depth = 0;
    LogStatus status = sort_by_time(log, &depth);
    if (status != LOG_OK) {
        printf("Cannot sort: %s\n", status_text(status));
        return;
    }
    view_reset(); /* записи переставлены, указатели смотрят не туда */
    printf("Records sorted: %ld, max recursion depth: %d\n", log->count, depth);
}

static void command_since(LogFile *log, char *words[], int count) {
    if (!require_log(log)) {
        return;
    }
    if (count < 3) {
        printf("Usage: since <YYYY-MM-DD> <hh:mm:ss>\n");
        return;
    }
    Timestamp moment = timestamp_parse(words[1], words[2]);
    if (moment == TIMESTAMP_INVALID) {
        printf("Cannot parse the date and time\n");
        return;
    }

    /* Двоичный поиск работает только на отсортированном массиве. */
    if (!log->sorted) {
        printf("The log is not sorted, sorting.\n");
        command_sort(log);
    }

    long index = search_not_before(log, moment);
    if (index == log->count) {
        printf("No records at or after this moment.\n");
        return;
    }
    printf("First matching record - number %ld of %ld:\n", index + 1, log->count);
    for (long i = index; i < log->count && i < index + LOGFILE_SHOW; ++i) {
        logfile_print_record(&log->records[i]);
    }
}

static void command_order(const LogFile *log, char *words[], int count) {
    if (!require_log(log)) {
        return;
    }
    Order order = ORDER_TIME;
    if (count >= 2) {
        if (strcmp(words[1], "level") == 0) {
            order = ORDER_LEVEL;
        } else if (strcmp(words[1], "module") == 0) {
            order = ORDER_MODULE;
        } else if (strcmp(words[1], "time") != 0) {
            printf("The order is time, level or module\n");
            return;
        }
    }
    LogStatus status = view_sort(log, order);
    if (status != LOG_OK) {
        printf("Cannot sort the pointers: %s\n", status_text(status));
        return;
    }
    printf("Pointers sorted: %ld, order %s.\n", view_count(), order_name(order));
    printf("The records themselves did not move: head still shows them\n");
    printf("in the order they were read from the file.\n");
}

static void command_view(const LogFile *log, char *words[], int count) {
    if (!require_log(log)) {
        return;
    }
    long n = count >= 2 ? strtol(words[1], NULL, 10) : LOGFILE_SHOW;
    if (n <= 0) {
        n = LOGFILE_SHOW;
    }
    /* Порядок ещё не выбран — по умолчанию по времени. */
    if (view_count() == 0 && view_sort(log, ORDER_TIME) != LOG_OK) {
        printf("Not enough memory for the view.\n");
        return;
    }
    view_print(n);
}

static void command_span(const LogFile *log) {
    if (!require_log(log)) {
        return;
    }
    Timestamp first = 0;
    Timestamp last = 0;
    if (!logfile_time_range(log, &first, &last)) {
        printf("No records.\n");
        return;
    }
    printf("Earliest: ");
    timestamp_print(first);
    printf("\nLatest:   ");
    timestamp_print(last);
    printf("\n");
}

/* Сравнение двух способов: переставлять указатели или сами записи.
   Сортировать сами записи журнала нельзя — порядок чтения из файла
   потеряется, — поэтому сортируется их копия в куче. */
static void command_cost(const LogFile *log) {
    if (!require_log(log)) {
        return;
    }
    clock_t start = clock();
    LogStatus status = view_sort(log, ORDER_TIME);
    double by_pointers = (double)(clock() - start) / CLOCKS_PER_SEC;

    /* +1: malloc от нуля байт вправе вернуть NULL, а пустой журнал — не ошибка. */
    LogRecord *copy = malloc((size_t)(log->count + 1) * sizeof(LogRecord));
    if (copy == NULL) {
        printf("Not enough memory to copy the records.\n");
        return;
    }
    for (long i = 0; i < log->count; ++i) {
        copy[i] = log->records[i];
    }
    /* Тот же алгоритм, но переставляются сами записи, а не указатели. */
    start = clock();
    if (status == LOG_OK) {
        status = sort_records_for_cost(copy, log->count);
    }
    double by_records = (double)(clock() - start) / CLOCKS_PER_SEC;
    /* Освобождается только массив копий. Тексты записей не трогаем:
       они принадлежат журналу, а в копии лежат те же адреса. */
    free(copy);
    if (status != LOG_OK) {
        printf("Cannot sort: %s\n", status_text(status));
        return;
    }

    printf("Records: %ld, size of one record: %u bytes\n",
           log->count, (unsigned)sizeof(LogRecord));
    printf("Sorting pointers:     %.4f s\n", by_pointers);
    printf("Sorting the records:  %.4f s\n", by_records);
}

/* Задание: загрузить и освободить журнал много раз подряд. Если что-то
   не освобождается, потребление памяти растёт с каждым кругом. */
static void command_stress(const LogFile *log, char *words[], int count) {
    if (!require_log(log)) {
        return;
    }
    long times = count >= 2 ? strtol(words[1], NULL, 10) : 20;
    if (times <= 0) {
        times = 20;
    }
    LogFile temp;
    logfile_init(&temp);
    long loaded = 0;
    int failed = 0;
    for (long i = 0; i < times && !failed; ++i) {
        if (logfile_load(&temp, log->path) == LOG_OK) {
            ++loaded;
        } else {
            failed = 1;
        }
        logfile_destroy(&temp);
        logfile_destroy(&temp); /* повторное освобождение безопасно */
    }
    printf("Logs loaded and freed: %ld of %ld\n", loaded, times);
}

/* Итог выборки: сколько записей нашлось и сколько из них показано. */
static void print_found(long found) {
    if (found == 0) {
        printf("Nothing found.\n");
    } else if (found > LOGFILE_SHOW) {
        printf("Records found: %ld, showing the first %d.\n", found, LOGFILE_SHOW);
    } else {
        printf("Records found: %ld.\n", found);
    }
}

static void command_grep(const LogFile *log, char *words[], int count, LevelMask mask) {
    if (!require_log(log)) {
        return;
    }
    if (count < 2) {
        printf("Search for what? Example: grep timeout\n");
        return;
    }
    print_found(query_text(log, mask, words[1]));
}

static void command_module(const LogFile *log, char *words[], int count, LevelMask mask) {
    if (!require_log(log)) {
        return;
    }
    if (count < 2) {
        printf("Which module? Example: module net\n");
        return;
    }
    print_found(query_module(log, mask, words[1]));
}

static void command_between(const LogFile *log, char *words[], int count, LevelMask mask) {
    if (!require_log(log)) {
        return;
    }
    if (count < 5) {
        printf("Usage: between <YYYY-MM-DD> <hh:mm:ss> <YYYY-MM-DD> <hh:mm:ss>\n");
        return;
    }
    Timestamp from = timestamp_parse(words[1], words[2]);
    Timestamp to = timestamp_parse(words[3], words[4]);
    if (from == TIMESTAMP_INVALID || to == TIMESTAMP_INVALID) {
        printf("Cannot parse the date and time\n");
        return;
    }
    if (from > to) {
        printf("The interval starts after it ends: no records.\n");
        return;
    }
    print_found(query_interval(log, mask, from, to));
}

/* Практика 8: двоичный журнал. Сохранение и загрузка печатают время,
   чтобы разницу с разбором текста было видно числами. */
static void command_bsave(const LogFile *log, char *words[], int count) {
    if (!require_log(log)) {
        return;
    }
    if (count < 2) {
        printf("Specify the file: bsave journal.bin\n");
        return;
    }
    clock_t start = clock();
    LogStatus status = binlog_save(log, words[1]);
    if (status != LOG_OK) {
        printf("Cannot save %s: %s\n", words[1], status_text(status));
        return;
    }
    printf("Saved %ld records in %.3f s\n",
           log->count, (double)(clock() - start) / CLOCKS_PER_SEC);
}

static void command_bload(LogFile *log, char *words[], int count) {
    if (count < 2) {
        printf("Specify the file: bload journal.bin\n");
        return;
    }
    clock_t start = clock();
    LogStatus status = binlog_load(log, words[1]);
    if (status != LOG_OK) {
        printf("Cannot read %s: %s\n", words[1], status_text(status));
        return;
    }
    view_reset(); /* указатели смотрели на записи прежнего журнала */
    printf("Read %ld records in %.3f s\n",
           log->count, (double)(clock() - start) / CLOCKS_PER_SEC);
}

void shell_run(LogFile *log) {
    if (log->loaded) {
        print_load_report(log);
    } else {
        printf("Log is not loaded.\n");
    }
    printf("Type help for the list of commands.\n");

    LevelMask mask = LEVEL_MASK_ALL;
    /* Конец ввода и команда quit сводятся к одному флагу: у цикла одно
       условие выхода, без break и continue. */
    int running = 1;
    while (running) {
        printf("> ");
        fflush(stdout);

        char line[SHELL_LINE_MAX];
        char *words[SHELL_WORDS_MAX];
        int count = read_words(line, sizeof line, words, SHELL_WORDS_MAX);
        const char *name = count > 0 ? words[0] : "";
        if (count < 0) {
            running = 0;
        } else if (count == 0) {
            /* Пустая строка — не ошибка: просто ждём следующую команду. */
        } else if (strcmp(name, "load") == 0) {
            command_load(log, words, count);
        } else if (strcmp(name, "bsave") == 0) {
            command_bsave(log, words, count);
        } else if (strcmp(name, "bload") == 0) {
            command_bload(log, words, count);
        } else if (strcmp(name, "gen") == 0) {
            command_gen(words, count);
        } else if (strcmp(name, "count") == 0) {
            command_count(log, mask);
        } else if (strcmp(name, "head") == 0) {
            if (require_log(log)) {
                logfile_print_head(log, LOGFILE_SHOW, mask);
            }
        } else if (strcmp(name, "tail") == 0) {
            if (require_log(log)) {
                logfile_print_tail(log, LOGFILE_SHOW, mask);
            }
        } else if (strcmp(name, "stats") == 0) {
            command_stats(log, REPORT_LEVELS);
        } else if (strcmp(name, "hours") == 0) {
            command_stats(log, REPORT_HOURS);
        } else if (strcmp(name, "modules") == 0) {
            command_stats(log, REPORT_MODULES);
        } else if (strcmp(name, "order") == 0) {
            command_order(log, words, count);
        } else if (strcmp(name, "view") == 0) {
            command_view(log, words, count);
        } else if (strcmp(name, "span") == 0) {
            command_span(log);
        } else if (strcmp(name, "cost") == 0) {
            command_cost(log);
        } else if (strcmp(name, "sort") == 0) {
            command_sort(log);
        } else if (strcmp(name, "since") == 0) {
            command_since(log, words, count);
        } else if (strcmp(name, "grep") == 0) {
            command_grep(log, words, count, mask);
        } else if (strcmp(name, "module") == 0) {
            command_module(log, words, count, mask);
        } else if (strcmp(name, "between") == 0) {
            command_between(log, words, count, mask);
        } else if (strcmp(name, "stress") == 0) {
            command_stress(log, words, count);
        } else if (strcmp(name, "level") == 0) {
            mask = parse_level_arguments(mask, words, count);
            level_mask_print(mask);
        } else if (strcmp(name, "filter") == 0) {
            level_mask_print(mask);
        } else if (strcmp(name, "time") == 0) {
            demo_time();
        } else if (strcmp(name, "help") == 0) {
            print_help();
        } else if (strcmp(name, "quit") == 0) {
            running = 0;
        } else {
            printf("Unknown command: %s\n", name);
            printf("Type help for the list of commands.\n");
        }
    }
    /* Перед выходом освобождаем всё, что выделяли модули сортировки. */
    view_free();
    sort_free();
    printf("Done.\n");
}
