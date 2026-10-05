#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "generator.h"
#include "level.h"
#include "shell.h"
#include "sort.h"
#include "stats.h"
#include "text.h"
#include "view.h"
#include "timestamp.h"

#define SHELL_LINE_MAX 256
#define SHELL_WORDS_MAX 8

static void print_help(void)
{
    printf("Available commands:\n");
    printf("  load <file>                           - load the log\n");
    printf("  gen <file> <number> [grain]           - create a test log\n");
    printf("  count                                 - how many records and rows\n");
    printf("  head                                  - first ten lines\n");
    printf("  tail                                  - last ten lines\n");
    printf("  order time|level|module               - viewing order (recordings don't move)\n");
    printf("  view [n]                              - show n records in the selected order\n");
    printf("  span                                  - the earliest and latest time\n");
    printf("  cost                                  - compare the sorting of pointers and records\n");
    printf("  sort                                  - arrange record by time\n");
    printf("  since <date> <time>                   - records start from the moment, for example since 2026-03-15 12:00:00\n");
    printf("  stats                                 - records by levels\n");
    printf("  hours                                 - distribution by hours\n");
    printf("  modules                               - entries by modules\n");
    printf("  grep <word>                           - entries with this word\n in the text\n");
    printf("  module <title>                        - entries of one module\n");
    printf("  between <date> <time> <date> <time>   - entries for the period\n");
    printf("  level <levels...>                     - set the filter, for example: level error fatal\n");
    printf("  level all | none                      - enable all levels or disable all\n");
    printf("  filter                                - show the current filter\n");
    printf("  time                                  - check time packaging\n");
    printf("  help                                  - this list\n");
    printf("  quit                                  - exit\n");
}

static int read_words(char *buffer, const size_t size, char **words, const int max_words) {
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

static void demo_time() {
    static const int SAMPLES[][6] = {
        { 2026,  3, 15, 23, 59, 59 },
        { 2026,  3, 16,  0,  0,  0 },
        { 2026, 12, 31, 23, 59, 59 },
        { 2027,  1,  1,  0,  0,  0 }
    };
    const size_t count = sizeof SAMPLES / sizeof SAMPLES[0];
    Timestamp previous = TIMESTAMP_INVALID;

    for (size_t i = 0; i < count; ++i) {
        const Timestamp value = timestamp_pack(SAMPLES[i][0], SAMPLES[i][1], SAMPLES[i][2],
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

static LevelMask apply_level_command(LevelMask mask, char **words, const int count) {
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

static void print_load_report(const LogFile *log) {
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

static int require_log(const LogFile *log) {
    if (!log->loaded) {
        printf("The log is not loaded. The load <file> or gen command.\n");
        return 0;
    }
    return 1;
}

static void do_load(LogFile *log, char **words, const int count) {
    if (count < 2) {
        printf("Specify the file: load <файл>\n");
        return;
    }
    const LogStatus status = logfile_load(log, words[1]);
    if (status != LOG_OK) {
        printf("Error: %s (%s)\n", logfile_status_text(status), words[1]);
        return;
    }
    print_load_report(log);
}

static void do_gen(char **words, const int count) {
    if (count < 3) {
        printf("Usage: gen <file> <count of lines> [grain]\n");
        return;
    }

    char *end;
    const long lines = strtol(words[2], &end, 10);
    if (*end != '\0' || lines <= 0) {
        printf("The number of lines must be a positive number\n");
        return;
    }

    unsigned long seed = 1;
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
    printf("Created %s: %ld lines, grain %lu\n", words[1], lines, seed);
}

static void do_order(const LogFile *log, char **words, const int count) {
    Order order = ORDER_TIME;

    if (count >= 2) {
        if (strcmp(words[1], "level") == 0) {
            order = ORDER_LEVEL;
        } else if (strcmp(words[1], "module") == 0) {
            order = ORDER_MODULE;
        } else if (strcmp(words[1], "time") != 0) {
            printf("The order can be time, level, or module\n");
            return;
        }
    }
    printf("Sorted pointers: %ld, order %s.\n",
           view_sort(log, order), order_name(order));
    printf("The records themselves did not move: head still shows them\n");
    printf("in the order in which they were read from the file.\n");
}

static void print_time(const Timestamp value) {
    int y, mo, d, h, mi, s;

    timestamp_unpack(value, &y, &mo, &d, &h, &mi, &s);
    printf("%04d-%02d-%02d %02d:%02d:%02d", y, mo, d, h, mi, s);
}

static void do_span(const LogFile *log)
{
    Timestamp first = 0;
    Timestamp last = 0;

    if (!logfile_time_range(log, &first, &last)) {
        printf("There are no records.\n");
        return;
    }
    printf("Earliest:  ");
    print_time(first);
    printf("\nLatest: ");
    print_time(last);
    printf("\n");
}

static LogRecord copy_space[LOGFILE_CAPACITY];

static void do_cost(const LogFile *log) {
    clock_t start = clock();
    view_sort(log, ORDER_TIME);
    const double by_pointers = (double) (clock() - start) / CLOCKS_PER_SEC;

    for (long i = 0; i < log->count; ++i) {
        copy_space[i] = log->records[i];
    }

    start = clock();
    sort_records_for_cost(copy_space, log->count);
    const double by_records = (double) (clock() - start) / CLOCKS_PER_SEC;

    printf("Records: %ld, size of one record: %u bytes\n",
           log->count, (unsigned)sizeof(LogRecord));
    printf("Sorting pointers: %.4f с\n", by_pointers);
    printf("Sorting the records themselves: %.4f с\n", by_records);
}

static int passes_filter(const LogRecord *record, const LevelMask mask) {
    return level_mask_has(mask, record->level);
}

static void print_found(const long shown, const long found) {
    if (found == 0) {
        printf("Nothing found.\n");
    } else if (found > shown) {
        printf("Records found: %ld, the first %ld are shown.\n", found, shown);
    } else {
        printf("Records found: %ld.\n", found);
    }
}

static void do_grep(const LogFile *log, char **words, const int count, const LevelMask mask) {
    long found = 0;
    long shown = 0;

    if (count < 2) {
        printf("What to look for? For example: grep timeout\n");
        return;
    }
    for (long i = 0; i < log->count; ++i) {
        const LogRecord *r = &log->records[i];

        if (!passes_filter(r, mask)) {
            continue;
        }
        if (text_find_ignore_case(r->text, words[1]) < 0) {
            continue;
        }
        ++found;
        if (shown < LOGFILE_SHOW) {
            logfile_print_record(r);
            ++shown;
        }
    }
    print_found(shown, found);
}

static void do_module(const LogFile *log, char **words, const int count, const LevelMask mask) {
    long found = 0;
    long shown = 0;
    long i;

    if (count < 2) {
        printf("Which module? For example: module net\n");
        return;
    }
    for (i = 0; i < log->count; ++i) {
        const LogRecord *r = &log->records[i];

        if (!passes_filter(r, mask) || !text_same_word(r->module, words[1])) {
            continue;
        }
        ++found;
        if (shown < LOGFILE_SHOW) {
            logfile_print_record(r);
            ++shown;
        }
    }
    print_found(shown, found);
}

static void do_between(const LogFile *log, char **words, const int count, const LevelMask mask) {
    if (count < 5) {
        printf("Usage: between <YYYY-MM-DD> <HH:MM:SS> <YYYY-MM-DD> <HH:MM:SS>\n");
        return;
    }
    int y1, mo1, d1, h1, mi1, s1;
    int y2, mo2, d2, h2, mi2, s2;
    if (sscanf(words[1], "%d-%d-%d", &y1, &mo1, &d1) != 3
        || sscanf(words[2], "%d:%d:%d", &h1, &mi1, &s1) != 3
        || sscanf(words[3], "%d-%d-%d", &y2, &mo2, &d2) != 3
        || sscanf(words[4], "%d:%d:%d", &h2, &mi2, &s2) != 3) {
        printf("Couldn't make out the date and time\n");
        return;
    }
    const Timestamp from = timestamp_pack(y1, mo1, d1, h1, mi1, s1);
    const Timestamp to = timestamp_pack(y2, mo2, d2, h2, mi2, s2);
    if (from == TIMESTAMP_INVALID || to == TIMESTAMP_INVALID) {
        printf("There is no such moment\n");
        return;
    }
    if (from > to) {
        printf("The beginning of the gap is later than its end: there are no entries.\n");
        return;
    }
    long found = 0, shown = 0;
    for (long i = 0; i < log->count; ++i) {
        const LogRecord *r = &log->records[i];

        if (!passes_filter(r, mask) || r->time < from || r->time > to) {
            continue;
        }
        ++found;
        if (shown < LOGFILE_SHOW) {
            logfile_print_record(r);
            ++shown;
        }
    }
    print_found(shown, found);
}

static void do_sort(LogFile *log) {
    const int depth = sort_by_time(log);
    printf("Entries sorted: %ld, maximum recursion depth: %d\n", log->count, depth);
}

static void do_since(LogFile *log, char **words, const int count) {
    if (count < 3) {
        printf("Usage: since <YYYY-MM-DD> <HH:MM:SS>\n");
        return;
    }

    int y, mo, d, h, mi, s;
    if (sscanf(words[1], "%d-%d-%d", &y, &mo, &d) != 3
        || sscanf(words[2], "%d:%d:%d", &h, &mi, &s) != 3) {
        printf("cannot to parse the date and time\n");
        return;
    }
    const Timestamp moment = timestamp_pack(y, mo, d, h, mi, s);
    if (moment == TIMESTAMP_INVALID) {
        printf("There is no such moment\n");
        return;
    }

    if (!log->sorted) {
        printf("The log is not sorted, I'm sorting.\n");
        do_sort(log);
    }

    const long index = search_not_before(log, moment);
    if (index == log->count) {
        printf("There is no record before this moment.\n");
        return;
    }
    printf("The first suitable entry is the %ld number from %ld:\n", index + 1, log->count);
    for (long i = index; i < log->count && i < index + LOGFILE_SHOW; ++i) {
        logfile_print_record(&log->records[i]);
    }
}

void shell_run(LogFile *log) {
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
    printf("Type 'help' for a list of commands.\n");

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
        } else if (strcmp(words[0], "grep") == 0) {
            if (require_log(log)) {
                do_grep(log, words, count, mask);
            }
        } else if (strcmp(words[0], "module") == 0) {
            if (require_log(log)) {
                do_module(log, words, count, mask);
            }
        } else if (strcmp(words[0], "between") == 0) {
            if (require_log(log)) {
                do_between(log, words, count, mask);
            }
        } else if (strcmp(words[0], "order") == 0) {
            if (require_log(log)) {
                do_order(log, words, count);
            }
        } else if (strcmp(words[0], "view") == 0) {
            if (require_log(log)) {
                long n = count >= 2 ? strtol(words[1], NULL, 10) : LOGFILE_SHOW;

                if (view_count() == 0) {
                    view_sort(log, ORDER_TIME);
                }
                view_print(n > 0 ? n : LOGFILE_SHOW);
            }
        } else if (strcmp(words[0], "span") == 0) {
            if (require_log(log)) {
                do_span(log);
            }
        } else if (strcmp(words[0], "cost") == 0) {
            if (require_log(log)) {
                do_cost(log);
            }
        } else if (strcmp(words[0], "sort") == 0) {
            if (require_log(log)) {
                do_sort(log);
            }
        } else if (strcmp(words[0], "since") == 0) {
            if (require_log(log)) {
                do_since(log, words, count);
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
