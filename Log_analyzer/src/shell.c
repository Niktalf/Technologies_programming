#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "generator.h"
#include "level.h"
#include "shell.h"
#include "stats.h"
#include "time_stamp.h"

#define SHELL_LINE_MAX 256
#define SHELL_WORDS_MAX 8

static void print_help(void)
{
    printf("Available commands:\n");
    printf("  load <load>                 - load the log\n");
    printf("  gen <file> <number> [grain] - create a test log\n");
    printf("  count                       - how many records and rows\n");
    printf("  head                        - first ten lines\n");
    printf("  tail                        - last ten lines\n");
    printf("  stats                       - records by levels\n");
    printf("  hours                       - distribution by hours\n");
    printf("  modules                     - entries by modules\n");
    printf("  level <levels...>           - set the filter, for example: level error fatal\n");
    printf("  level all | none            - enable all levels or disable all\n");
    printf("  filter                      - show the current filter\n");
    printf("  time                        - check time packaging\n");
    printf("  help                        - this list\n");
    printf("  quit                        - exit\n");
}

static int read_words(char *buffer, const size_t size, char **words, const int max_words)
{
    if (fgets(buffer, (int)size, stdin) == NULL) {
        return -1;
    }
    if (strchr(buffer, '\n') == NULL) {
        int c;
        while ((c = getchar()) != EOF && c != '\n') {}
    }

    char *p = buffer;
    int count = 0;
    while (*p != '\0' && count < max_words) {
        while (*p != '\0' && isspace((unsigned char)*p)) {
            *p++ = '\0';
        }
        if (*p == '\0') {
            break;
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

static void demo_time()
{
    static const int SAMPLES[][6] = {
        { 2026,  3, 15, 23, 59, 59 },
        { 2026,  3, 16,  0,  0,  0 },
        { 2026, 12, 31, 23, 59, 59 },
        { 2027,  1,  1,  0,  0,  0 }
    };
    const size_t count = sizeof SAMPLES / sizeof SAMPLES[0];
    TimeStamp previous = TIMESTAMP_INVALID;

    for (size_t i = 0; i < count; ++i) {
        const TimeStamp value = timestamp_pack(SAMPLES[i][0], SAMPLES[i][1], SAMPLES[i][2],
                                         SAMPLES[i][3], SAMPLES[i][4], SAMPLES[i][5]);
        int y = 0, mo = 0, d = 0, h = 0, mi = 0, s = 0;

        timestamp_unpack(value, &y, &mo, &d, &h, &mi, &s);
        printf("%04d-%02d-%02d %02d:%02d:%02d -> %lld -> %04d-%02d-%02d %02d:%02d:%02d  час %d\n",
               SAMPLES[i][0], SAMPLES[i][1], SAMPLES[i][2],
               SAMPLES[i][3], SAMPLES[i][4], SAMPLES[i][5],
               (long long)value, y, mo, d, h, mi, s, timestamp_hour(value));

        if (previous != TIMESTAMP_INVALID && value <= previous) {
            printf("  ERROR: the order is violated\n");
        }
        previous = value;
    }
    printf("Invalid date 2026-13-01: %lld (expected %lld)\n",
           (long long)timestamp_pack(2026, 13, 1, 0, 0, 0), (long long)TIMESTAMP_INVALID);
}

static LevelMask apply_level_command(LevelMask mask, char **words, int count)
{
    if (count < 2) {
        printf("level requires at least one level, all or none\n");
        return mask;
    }
    if (strcmp(words[1], "all") == 0) {
        return LEVEL_MASK_ALL;
    }
    if (strcmp(words[1], "none") == 0) {
        return LEVEL_MASK_NONE;
    }

    mask = LEVEL_MASK_NONE;
    for (int i = 1; i < count; ++i) {
        const Level level = level_from_word(words[i]);

        if (level == LEVEL_UNKNOWN) {
            printf("Unknown level: %s\n", words[i]);
            continue;
        }
        mask = level_mask_set(mask, level);
    }
    return mask;
}

static void print_load_report(const LogFile *log)
{
    printf("File: %s\n", log->path);
    printf("Line: %ld, entries: %ld\n", log->lines, log->count);
    if (log->broken > 0) {
        printf("Do not parse the lines: %ld\n", log->broken);
    }
    if (log->overflow > 0) {
        printf("Couldn't fit in the record memory: %ld (capacity %d)\n",
               log->overflow, LOGFILE_CAPACITY);
    }
    if (log->truncated > 0) {
        printf("Line longer than buffer: %ld\n", log->truncated);
    }
}

static int require_log(const LogFile *log)
{
    if (!log->loaded) {
        printf("The log is not loaded. The load <file> or gen command.\n");
        return 0;
    }
    return 1;
}

static void do_load(LogFile *log, char **words, int count)
{
    if (count < 2) {
        printf("Specify the file: load <файл>\n");
        return;
    }
    LogStatus status = logfile_load(log, words[1]);
    if (status != LOG_OK) {
        printf("Error: %s (%s)\n", logfile_status_text(status), words[1]);
        return;
    }
    print_load_report(log);
}

static void do_gen(char **words, const int count)
{
    unsigned long seed = 1;
    char *end;

    if (count < 3) {
        printf("Usage: gen <file> <count of lines> [grain]\n");
        return;
    }
    const long lines = strtol(words[2], &end, 10);
    if (*end != '\0' || lines <= 0) {
        printf("The count of lines must be a positive number\n");
        return;
    }
    if (count >= 4) {
        seed = strtoul(words[3], &end, 10);
        if (*end != '\0') {
            printf("Grain must be a number\n");
            return;
        }
    }
    if (!generator_write(words[1], lines, (unsigned int)seed)) {
        printf("Failed to create file: %s\n", words[1]);
        return;
    }
    printf("Create %s: %ld lines, grain %lu\n", words[1], lines, seed);
}

void shell_run(LogFile *log)
{
    char line[SHELL_LINE_MAX];
    char *words[SHELL_WORDS_MAX];
    LevelMask mask = LEVEL_MASK_ALL;
    Stats stats;
    int running = 1;

    if (log->loaded) {
        print_load_report(log);
    } else {
        printf("The log is not loaded.\n");
    }
    printf("Type help for a list of commands.\n");

    while (running) {
        printf("> ");
        fflush(stdout);

        const int count = read_words(line, sizeof line, words, SHELL_WORDS_MAX);
        if (count < 0) {
            break;
        }
        if (count == 0) {
            continue;
        }

        if (strcmp(words[0], "load") == 0) {
            do_load(log, words, count);
        } else if (strcmp(words[0], "gen") == 0) {
            do_gen(words, count);
        } else if (strcmp(words[0], "count") == 0) {
            if (require_log(log)) {
                printf("Entries: %ld, lines in file: %ld\n", log->count, log->lines);
            }
        } else if (strcmp(words[0], "head") == 0) {
            if (require_log(log)) {
                logfile_print_head(log, LOGFILE_SHOW);
            }
        } else if (strcmp(words[0], "tail") == 0) {
            if (require_log(log)) {
                logfile_print_tail(log, LOGFILE_SHOW);
            }
        } else if (strcmp(words[0], "stats") == 0) {
            if (require_log(log)) {
                stats_collect(log, &stats);
                stats_print_levels(&stats, log->count);
            }
        } else if (strcmp(words[0], "hours") == 0) {
            if (require_log(log)) {
                stats_collect(log, &stats);
                stats_print_hours(&stats);
            }
        } else if (strcmp(words[0], "modules") == 0) {
            if (require_log(log)) {
                stats_collect(log, &stats);
                stats_print_modules(&stats);
            }
        } else if (strcmp(words[0], "level") == 0) {
            mask = apply_level_command(mask, words, count);
            level_mask_print(mask);
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
        }
    }
    printf("The work is completed.\n");
}
