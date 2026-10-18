#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "level.h"
#include "shell.h"
#include "timestamp.h"

#define SHELL_LINE_MAX  128
#define SHELL_WORDS_MAX 8

static void print_help(void) {
    printf("Available commands:\n");
    printf("  count              - number of lines in the log\n");
    printf("  head               - first ten lines\n");
    printf("  tail               - last ten lines\n");
    printf("  level <levels...>  - set the filter, e.g. level error fatal\n");
    printf("  level all | none   - enable all levels or clear them all\n");
    printf("  filter             - show the current filter\n");
    printf("  time               - check the time packing\n");
    printf("  help               - this list\n");
    printf("  quit               - exit\n");
}

/* Дочитывает и выбрасывает остаток строки до перевода строки или конца ввода. */
static void skip_rest_of_line(void) {
    int c = getchar();
    while (c != '\n' && c != EOF) {
        c = getchar();
    }
}

/* Разбивает строку на слова по пробелам и приводит их к нижнему регистру.
   Пробелы заменяются нулями, в words попадают указатели внутрь строки.
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
        ++count;
        while (*p != '\0' && !isspace((unsigned char)*p)) {
            *p = (char)tolower((unsigned char)*p);
            ++p;
        }
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

/* Команда level: новая маска и, если она изменилась, новый проход по файлу.
   Весь файл не хранится, поэтому иначе первые и последние строки не получить. */
static void command_level(LogFile *log, LevelMask *mask, char *words[], int count) {
    *mask = parse_level_arguments(*mask, words, count);
    level_mask_print(*mask);
    if (*mask == log->mask) {
        return;
    }
    LogStatus status = logfile_scan(log, log->path, *mask);
    if (status != LOG_OK) {
        printf("Cannot re-read the file: %s\n", logfile_status_text(status));
        return;
    }
    printf("Lines passing the filter: %ld of %ld\n", log->matched, log->total);
}

void shell_run(LogFile *log) {
    printf("Lines loaded: %ld\n", log->total);
    if (log->truncated > 0) {
        printf("Lines too long and truncated: %ld\n", log->truncated);
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
        if (count < 0) {
            running = 0;
        } else if (count == 0) {
            /* Пустая строка — не ошибка: просто ждём следующую команду. */
        } else if (strcmp(words[0], "count") == 0) {
            printf("Lines in the log: %ld, passing the filter: %ld, with unknown level: %ld\n",
                   log->total, log->matched, log->unknown);
        } else if (strcmp(words[0], "head") == 0) {
            logfile_print_head(log, LOGFILE_HEAD_SIZE);
        } else if (strcmp(words[0], "tail") == 0) {
            logfile_print_tail(log, LOGFILE_TAIL_SIZE);
        } else if (strcmp(words[0], "level") == 0) {
            command_level(log, &mask, words, count);
        } else if (strcmp(words[0], "filter") == 0) {
            level_mask_print(mask);
        } else if (strcmp(words[0], "time") == 0) {
            demo_time();
        } else if (strcmp(words[0], "help") == 0) {
            print_help();
        } else if (strcmp(words[0], "quit") == 0) {
            running = 0;
        } else {
            printf("Unknown command: %s\n", words[0]);
            printf("Type help for the list of commands.\n");
        }
    }
    printf("Done.\n");
}
