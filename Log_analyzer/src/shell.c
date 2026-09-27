#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "level.h"
#include "shell.h"
#include "time_stamp.h"

#define SHELL_LINE_MAX 128
#define SHELL_WORDS_MAX 8

static void print_help(void)
{
    printf("Available commands:\n");
    printf("  count             - number of lines in the log\n");
    printf("  head              - first ten lines\n");
    printf("  tail              - last ten lines\n");
    printf("  level <levels...> -  set the filter, for example: level error fatal\n");
    printf("  level all | none  - enable all levels or disable all\n");
    printf("  filter            - show the current filter\n");
    printf("  time              - check time packaging\n");
    printf("  help              - this list\n");
    printf("  quit              - exit\n");
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
        words[count++] = p;
        while (*p != '\0' && !isspace((unsigned char)*p)) {
            *p = (char)tolower((unsigned char)*p);
            ++p;
        }
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
        TimeStamp value = timestamp_pack(SAMPLES[i][0], SAMPLES[i][1], SAMPLES[i][2],
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

void shell_run(const LogFile *log)
{
    char line[SHELL_LINE_MAX];
    char *words[SHELL_WORDS_MAX];
    LevelMask mask = LEVEL_MASK_ALL;
    int running = 1;

    printf("Loaded lines: %ld\n", log->total);
    if (log->truncated > 0) {
        printf("Truncated too long lines: %ld\n", log->truncated);
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

        if (strcmp(words[0], "count") == 0) {
            printf("Count of lines in the log: %ld\n", log->total);
        } else if (strcmp(words[0], "head") == 0) {
            logfile_print_head(log, LOGFILE_HEAD_SIZE);
        } else if (strcmp(words[0], "tail") == 0) {
            logfile_print_tail(log, LOGFILE_TAIL_SIZE);
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
