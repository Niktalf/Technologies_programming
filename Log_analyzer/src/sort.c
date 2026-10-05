#include "sort.h"

static LogRecord buffer[LOGFILE_CAPACITY];

static void merge(LogRecord *records, const long left, const long middle, const long right) {
    long i = left;
    long j = middle;
    long k = left;

    while (i < middle && j < right) {
        if (records[i].time <= records[j].time) {
            buffer[k++] = records[i++];
        } else {
            buffer[k++] = records[j++];
        }
    }
    while (i < middle) {
        buffer[k++] = records[i++];
    }
    while (j < right) {
        buffer[k++] = records[j++];
    }
    for (k = left; k < right; ++k) {
        records[k] = buffer[k];
    }
}

static void merge_sort(LogRecord *records, const long left, const long right, const int depth, int *max_depth) {
    if (depth > *max_depth) {
        *max_depth = depth;
    }
    if (right - left < 2) {
        return;
    }
    const long middle = left + (right - left) / 2;

    merge_sort(records, left, middle, depth + 1, max_depth);
    merge_sort(records, middle, right, depth + 1, max_depth);
    merge(records, left, middle, right);
}

void sort_records_for_cost(LogRecord *records, const long count) {
    int depth = 0;
    merge_sort(records, 0, count, 1, &depth);
}

int sort_by_time(LogFile *log) {
    int max_depth = 0;

    merge_sort(log->records, 0, log->count, 1, &max_depth);
    log->sorted = 1;
    return max_depth;
}

static long lower_bound(const LogRecord *records, const long low, const long high, const Timestamp moment) {
    if (low >= high) {
        return low;
    }
    const long middle = low + (high - low) / 2;

    if (records[middle].time < moment) {
        return lower_bound(records, middle + 1, high, moment);
    }
    return lower_bound(records, low, middle, moment);
}

long search_not_before(const LogFile *log, const Timestamp moment) {
    return lower_bound(log->records, 0, log->count, moment);
}
