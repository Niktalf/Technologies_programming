#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "generator.h"
#include "level.h"
#include "shell.h"
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
