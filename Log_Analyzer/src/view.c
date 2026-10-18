#include <stddef.h>
#include <string.h>

#include "view.h"

/* Массив указателей на записи журнала. Сами записи лежат в журнале
   и никуда не перемещаются. Массивы статические: они размером с журнал. */
static const LogRecord *items[LOGFILE_CAPACITY];
static const LogRecord *buffer[LOGFILE_CAPACITY];
static long count;

const char *order_name(Order order) {
    switch (order) {
    case ORDER_TIME:
        return "by time";
    case ORDER_LEVEL:
        return "by level";
    case ORDER_MODULE:
        return "by module";
    default:
        return "unknown order";
    }
}

static int compare_time(const LogRecord *a, const LogRecord *b) {
    if (a->time < b->time) {
        return -1;
    }
    if (a->time > b->time) {
        return 1;
    }
    return 0;
}

/* Сравнение двух записей по выбранному ключу.
   Возвращает отрицательное число, ноль или положительное — как strcmp.
   При равном ключе записи сравниваются по времени: иначе порядок внутри
   уровня или модуля был бы случайным. */
static int compare(const LogRecord *a, const LogRecord *b, Order order) {
    if (order == ORDER_LEVEL && a->level != b->level) {
        return (int)a->level - (int)b->level;
    }
    if (order == ORDER_MODULE) {
        int result = strcmp(a->module, b->module);
        if (result != 0) {
            return result;
        }
    }
    return compare_time(a, b);
}

static void merge(long left, long middle, long right, Order order) {
    long i = left;
    long j = middle;
    long k = left;
    while (i < middle && j < right) {
        if (compare(items[i], items[j], order) <= 0) {
            buffer[k] = items[i];
            ++i;
        } else {
            buffer[k] = items[j];
            ++j;
        }
        ++k;
    }
    while (i < middle) {
        buffer[k] = items[i];
        ++i;
        ++k;
    }
    while (j < right) {
        buffer[k] = items[j];
        ++j;
        ++k;
    }
    /* Копируются указатели, а не записи: восемь байт вместо трёхсот. */
    for (long n = left; n < right; ++n) {
        items[n] = buffer[n];
    }
}

static void merge_sort(long left, long right, Order order) {
    if (right - left < 2) {
        return;
    }
    long middle = left + (right - left) / 2;
    merge_sort(left, middle, order);
    merge_sort(middle, right, order);
    merge(left, middle, right, order);
}

long view_sort(const LogFile *log, Order order) {
    count = log->count;
    for (long i = 0; i < count; ++i) {
        /* Берём адрес записи, а не её копию. */
        items[i] = &log->records[i];
    }
    merge_sort(0, count, order);
    return count;
}

void view_print(long count_to_print) {
    long limit = count_to_print < count ? count_to_print : count;
    for (long i = 0; i < limit; ++i) {
        logfile_print_record(items[i]);
    }
}

const LogRecord *view_at(long index) {
    if (index < 0 || index >= count) {
        return NULL;
    }
    return items[index];
}

long view_count(void) {
    return count;
}

void view_reset(void) {
    count = 0;
}
