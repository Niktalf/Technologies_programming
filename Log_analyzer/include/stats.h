#ifndef STATS_H
#define STATS_H

#include "log_file.h"

#define STATS_MODULES_MAX 64

typedef struct {
    long by_level[LEVEL_COUNT];
    long by_hour[24];
    char modules[STATS_MODULES_MAX][LOGFILE_MODULE_MAX];
    long module_counts[STATS_MODULES_MAX];
    int  module_count;
    long modules_overflow;
} Stats;

void stats_collect(const LogFile *log, Stats *stats);

void stats_print_levels(const Stats *stats, long total);
void stats_print_hours(const Stats *stats);
void stats_print_modules(const Stats *stats);

#endif // STATS_H
