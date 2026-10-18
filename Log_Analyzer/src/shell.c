#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "generator.h"
#include "level.h"
#include "shell.h"
#include "sort.h"
#include "text.h"
#include "view.h"
#include "stats.h"
#include "timestamp.h"

#define SHELL_LINE_MAX  256
#define SHELL_WORDS_MAX 8

static void print_help(void) {
    printf("Available commands:\n");
    printf("  load <file>                - load a log\n");
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
    static const int SAMPLES[][6] = {
        { 2026, 3, 15, 23, 59, 59 },
        { 2026, 3, 16, 0, 0, 0 },
        { 2026, 12, 31, 23, 59, 59 },
        { 2027, 1, 1, 0, 0, 0 }
    };
    const size_t count = sizeof SAMPLES / sizeof SAMPLES[0];

    Timestamp previous = TIMESTAMP_INVALID;
    for (size_t i = 0; i < count; ++i) {
        const int *t = SAMPLES[i];
        Timestamp value = timestamp_pack(t[0], t[1], t[2], t[3], t[4], t[5]);

        int year = 0;
        int month = 0;
        int day = 0;
        int hour = 0;
        int minute = 0;
        int second = 0;
        timestamp_unpack(value, &year, &month, &day, &hour, &minute, &second);

        printf("%04d-%02d-%02d %02d:%02d:%02d -> %lld -> ",
               t[0], t[1], t[2], t[3], t[4], t[5], (long long)value);
        printf("%04d-%02d-%02d %02d:%02d:%02d  hour %d\n",
               year, month, day, hour, minute, second, timestamp_hour(value));

        if (previous != TIMESTAMP_INVALID && value <= previous) {
            printf("  ERROR: order is broken\n");
        }
        previous = value;
    }
    printf("Invalid date 2026-13-01: %lld (expected %lld)\n",
           (long long)timestamp_pack(2026, 13, 1, 0, 0, 0), (long long)TIMESTAMP_INVALID);
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
    if (log->overflow > 0) {
        printf("Records that did not fit into memory: %ld (capacity %d)\n",
               log->overflow, LOGFILE_CAPACITY);
    }
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
    LogStatus status = logfile_load(log, words[1]);
    if (status != LOG_OK) {
        printf("Error: %s (%s)\n", logfile_status_text(status), words[1]);
        return;
    }
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
    if (!generator_write(words[1], lines, (unsigned int)seed)) {
        printf("Cannot create the file: %s\n", words[1]);
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
    int depth = sort_by_time(log);
    view_reset(); /* записи переставлены, указатели смотрят не туда */
    printf("Records sorted: %ld, max recursion depth: %d\n", log->count, depth);
}

/* Разбирает «ГГГГ-ММ-ДД» и «чч:мм:сс» в момент времени.
   Возвращает TIMESTAMP_INVALID, если разобрать не удалось. */
static Timestamp parse_moment(const char *date, const char *time) {
    int year = 0;
    int month = 0;
    int day = 0;
    int hour = 0;
    int minute = 0;
    int second = 0;
    if (sscanf(date, "%d-%d-%d", &year, &month, &day) != 3
        || sscanf(time, "%d:%d:%d", &hour, &minute, &second) != 3) {
        return TIMESTAMP_INVALID;
    }
    return timestamp_pack(year, month, day, hour, minute, second);
}

static void command_since(LogFile *log, char *words[], int count) {
    if (!require_log(log)) {
        return;
    }
    if (count < 3) {
        printf("Usage: since <YYYY-MM-DD> <hh:mm:ss>\n");
        return;
    }
    Timestamp moment = parse_moment(words[1], words[2]);
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
    printf("Pointers sorted: %ld, order %s.\n", view_sort(log, order), order_name(order));
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
    if (view_count() == 0) {
        view_sort(log, ORDER_TIME);
    }
    view_print(n);
}

static void print_time(Timestamp value) {
    int year = 0;
    int month = 0;
    int day = 0;
    int hour = 0;
    int minute = 0;
    int second = 0;
    timestamp_unpack(value, &year, &month, &day, &hour, &minute, &second);
    printf("%04d-%02d-%02d %02d:%02d:%02d", year, month, day, hour, minute, second);
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
    print_time(first);
    printf("\nLatest:   ");
    print_time(last);
    printf("\n");
}

/* Копия записей для замера: сортировать сами записи журнала нельзя,
   иначе порядок чтения из файла потеряется. Статическая: она размером с журнал. */
static LogRecord copy_space[LOGFILE_CAPACITY];

/* Сравнение двух способов: переставлять указатели или сами записи. */
static void command_cost(const LogFile *log) {
    if (!require_log(log)) {
        return;
    }
    clock_t start = clock();
    view_sort(log, ORDER_TIME);
    double by_pointers = (double)(clock() - start) / CLOCKS_PER_SEC;

    for (long i = 0; i < log->count; ++i) {
        copy_space[i] = log->records[i];
    }
    /* Тот же алгоритм, но переставляются сами записи, а не указатели. */
    start = clock();
    sort_records_for_cost(copy_space, log->count);
    double by_records = (double)(clock() - start) / CLOCKS_PER_SEC;

    printf("Records: %ld, size of one record: %u bytes\n",
           log->count, (unsigned)sizeof(LogRecord));
    printf("Sorting pointers:     %.4f s\n", by_pointers);
    printf("Sorting the records:  %.4f s\n", by_records);
}

/* Условие выборки: функция получает запись и данные условия (образец,
   имя модуля, границы интервала) и отвечает, подходит ли запись. */
typedef int (*Match)(const LogRecord *record, const void *condition);

/* Печатает первые LOGFILE_SHOW записей, которые проходят фильтр уровней
   и условие match, и сообщает, сколько их всего. Все выборки учитывают
   фильтр, заданный командой level: именно для этого он заводился
   на практике 2. Один цикл на три команды — вместо трёх одинаковых. */
static void print_matching(const LogFile *log, LevelMask mask, Match match,
                           const void *condition) {
    long found = 0;
    for (long i = 0; i < log->count; ++i) {
        const LogRecord *r = &log->records[i];
        if (level_mask_has(mask, r->level) && match(r, condition)) {
            if (found < LOGFILE_SHOW) {
                logfile_print_record(r);
            }
            ++found;
        }
    }
    if (found == 0) {
        printf("Nothing found.\n");
    } else if (found > LOGFILE_SHOW) {
        printf("Records found: %ld, showing the first %d.\n", found, LOGFILE_SHOW);
    } else {
        printf("Records found: %ld.\n", found);
    }
}

static int text_contains(const LogRecord *record, const void *condition) {
    return text_find_ignore_case(record->text, condition) >= 0;
}

static int module_is(const LogRecord *record, const void *condition) {
    return text_same_word(record->module, condition);
}

/* Границы интервала, обе включительно. */
typedef struct {
    Timestamp from;
    Timestamp to;
} Interval;

static int time_inside(const LogRecord *record, const void *condition) {
    const Interval *interval = condition;
    return record->time >= interval->from && record->time <= interval->to;
}

static void command_grep(const LogFile *log, char *words[], int count, LevelMask mask) {
    if (!require_log(log)) {
        return;
    }
    if (count < 2) {
        printf("Search for what? Example: grep timeout\n");
        return;
    }
    print_matching(log, mask, text_contains, words[1]);
}

static void command_module(const LogFile *log, char *words[], int count, LevelMask mask) {
    if (!require_log(log)) {
        return;
    }
    if (count < 2) {
        printf("Which module? Example: module net\n");
        return;
    }
    print_matching(log, mask, module_is, words[1]);
}

static void command_between(const LogFile *log, char *words[], int count, LevelMask mask) {
    if (!require_log(log)) {
        return;
    }
    if (count < 5) {
        printf("Usage: between <YYYY-MM-DD> <hh:mm:ss> <YYYY-MM-DD> <hh:mm:ss>\n");
        return;
    }
    Interval interval;
    interval.from = parse_moment(words[1], words[2]);
    interval.to = parse_moment(words[3], words[4]);
    if (interval.from == TIMESTAMP_INVALID || interval.to == TIMESTAMP_INVALID) {
        printf("Cannot parse the date and time\n");
        return;
    }
    if (interval.from > interval.to) {
        printf("The interval starts after it ends: no records.\n");
        return;
    }
    print_matching(log, mask, time_inside, &interval);
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
    printf("Done.\n");
}
