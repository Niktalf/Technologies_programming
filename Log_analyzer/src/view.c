#include <string.h>

#include "view.h"

static const LogRecord *items[LOGFILE_CAPACITY];
static const LogRecord *buffer[LOGFILE_CAPACITY];
static long count;

const char *order_name(const Order order) {
    switch (order) {
        case ORDER_TIME:   return "by time";
        case ORDER_LEVEL:  return "by level";
        case ORDER_MODULE: return "by module";
        default:           return "unknown order";
    }
}

static int compare(const LogRecord *a, const LogRecord *b, const Order order) {
    switch (order) {
    case ORDER_LEVEL:
        if (a->level != b->level) {
            return (int)a->level - (int)b->level;
        }
        break;
    case ORDER_MODULE: {
        const int result = strcmp(a->module, b->module);
        if (result != 0) {
            return result;
        }
        break;
    }
    case ORDER_TIME:
    default:
        break;
    }
    if (a->time < b->time) {
        return -1;
    }
    if (a->time > b->time) {
        return 1;
    }
    return 0;
}

static void merge(const long left, const long middle, const long right, const Order order) {
    long i = left;
    long j = middle;
    long k = left;

    while (i < middle && j < right) {
        if (compare(items[i], items[j], order) <= 0) {
            buffer[k++] = items[i++];
        } else {
            buffer[k++] = items[j++];
        }
    }
    while (i < middle) {
        buffer[k++] = items[i++];
    }
    while (j < right) {
        buffer[k++] = items[j++];
    }
    for (k = left; k < right; ++k) {
        items[k] = buffer[k];
    }
}

static void merge_sort(const long left, const long right, const Order order) {
    if (right - left < 2) {
        return;
    }
    const long middle = left + (right - left) / 2;

    merge_sort(left, middle, order);
    merge_sort(middle, right, order);
    merge(left, middle, right, order);
}

long view_sort(const LogFile *log, const Order order) {
    count = log->count;
    for (long i = 0; i < count; ++i) {
        items[i] = &log->records[i];
    }

    merge_sort(0, count, order);
    return count;
}

void view_print(const long count_to_print) {
    const long limit = count_to_print < count ? count_to_print : count;

    for (long i = 0; i < limit; ++i) {
        logfile_print_record(items[i]);
    }
}

const LogRecord *view_at(const long index) {
    if (index < 0 || index >= count) {
        return NULL;
    }
    return items[index];
}

long view_count() {
    return count;
}
