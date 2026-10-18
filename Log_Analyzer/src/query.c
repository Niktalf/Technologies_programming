#include <stdio.h>

#include "query.h"
#include "text.h"

/* Условие выборки: функция получает запись и данные условия (образец,
   имя модуля, границы интервала) и отвечает, подходит ли запись. */
typedef int (*Match)(const LogRecord *record, const void *condition);

/* Один цикл на все выборки — вместо трёх одинаковых. */
static long print_matching(const LogFile *log, LevelMask mask, Match match,
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
    return found;
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

long query_text(const LogFile *log, LevelMask mask, const char *word) {
    return print_matching(log, mask, text_contains, word);
}

long query_module(const LogFile *log, LevelMask mask, const char *module) {
    return print_matching(log, mask, module_is, module);
}

long query_interval(const LogFile *log, LevelMask mask, Timestamp from, Timestamp to) {
    Interval interval = { from, to };
    return print_matching(log, mask, time_inside, &interval);
}
