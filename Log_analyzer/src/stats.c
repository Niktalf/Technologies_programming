#include <stdio.h>
#include <string.h>

#include "stats.h"

#define BAR_WIDTH 50

static int module_index(Stats *stats, const char *module) {
    for (int i = 0; i < stats->module_count; ++i) {
        if (strcmp(stats->modules[i], module) == 0) {
            return i;
        }
    }
    if (stats->module_count >= STATS_MODULES_MAX) {
        return -1;
    }
    strcpy(stats->modules[stats->module_count], module);
    stats->module_counts[stats->module_count] = 0;
    return stats->module_count++;
}

void stats_collect(const LogFile *log, Stats *stats) {
    memset(stats, 0, sizeof *stats);
    for (long i = 0; i < log->count; ++i) {
        const LogRecord *r = &log->records[i];
        const int hour = timestamp_hour(r->time);

        ++stats->by_level[r->level];
        if (hour >= 0 && hour < 24) {
            ++stats->by_hour[hour];
        }
        const int m = module_index(stats, r->module);
        if (m >= 0) {
            ++stats->module_counts[m];
        } else {
            ++stats->modules_overflow;
        }
    }
}

void stats_print_levels(const Stats *stats, const long total) {
    for (int i = 0; i < LEVEL_COUNT; ++i) {
        const long percent = total > 0 ? stats->by_level[i] * 100 / total : 0;
        printf("  %-5s %7ld  %3ld%%\n", level_name((Level)i), stats->by_level[i], percent);
    }
}

void stats_print_hours(const Stats *stats) {
    long peak = 0;
    int h;

    for (h = 0; h < 24; ++h) {
        if (stats->by_hour[h] > peak) {
            peak = stats->by_hour[h];
        }
    }
    for (h = 0; h < 24; ++h) {
        const int bar = peak > 0 ? (int)(stats->by_hour[h] * BAR_WIDTH / peak) : 0;

        printf("  %02d  %6ld  ", h, stats->by_hour[h]);
        for (int k = 0; k < bar; ++k) {
            putchar('#');
        }
        putchar('\n');
    }
}

void stats_print_modules(const Stats *stats) {
    for (int i = 0; i < stats->module_count; ++i) {
        printf("  %-12s %7ld\n", stats->modules[i], stats->module_counts[i]);
    }
    if (stats->modules_overflow > 0) {
        printf("  (didn't fit in the module table: %ld entries)\n", stats->modules_overflow);
    }
}
