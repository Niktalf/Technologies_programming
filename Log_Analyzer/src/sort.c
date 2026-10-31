#include <stdlib.h>

#include "sort.h"

/* Временный массив для слияния — того же размера, что и журнал. Он живёт
   в куче между сортировками и растёт, когда журнал стал больше. */
static LogRecord *buffer;
static long buffer_capacity;

/* Готовит буфер хотя бы на needed записей. Возвращает 1 при успехе. */
static int reserve_buffer(long needed) {
    if (needed <= buffer_capacity) {
        return 1;
    }
    LogRecord *bigger = realloc(buffer, (size_t)needed * sizeof(LogRecord));
    if (bigger == NULL) {
        return 0;
    }
    buffer = bigger;
    buffer_capacity = needed;
    return 1;
}

void sort_free(void) {
    free(buffer);
    buffer = NULL;
    buffer_capacity = 0;
}

/* Сливает две отсортированные половины [left, middle) и [middle, right).
   Результат пишется во временный массив, а не туда, откуда читаем,
   иначе ещё не прочитанные записи будут затёрты. */
static void merge(LogRecord *records, long left, long middle, long right) {
    long i = left;
    long j = middle;
    long k = left;
    while (i < middle && j < right) {
        /* Нестрогое сравнение сохраняет устойчивость: при равном времени
           первой идёт запись из левой половины. */
        if (records[i].time <= records[j].time) {
            buffer[k] = records[i];
            ++i;
        } else {
            buffer[k] = records[j];
            ++j;
        }
        ++k;
    }
    /* Одна из половин кончилась; остаток другой уже упорядочен. */
    while (i < middle) {
        buffer[k] = records[i];
        ++i;
        ++k;
    }
    while (j < right) {
        buffer[k] = records[j];
        ++j;
        ++k;
    }
    for (long n = left; n < right; ++n) {
        records[n] = buffer[n];
    }
}

static void merge_sort(LogRecord *records, long left, long right, int depth, int *max_depth) {
    if (depth > *max_depth) {
        *max_depth = depth;
    }
    /* Условие остановки: ноль или один элемент уже отсортированы. */
    if (right - left < 2) {
        return;
    }
    long middle = left + (right - left) / 2;
    merge_sort(records, left, middle, depth + 1, max_depth);
    merge_sort(records, middle, right, depth + 1, max_depth);
    merge(records, left, middle, right);
}

int sort_by_time(LogFile *log) {
    if (!reserve_buffer(log->count)) {
        return 0;
    }
    int max_depth = 0;
    merge_sort(log->records, 0, log->count, 1, &max_depth);
    log->sorted = 1;
    return max_depth;
}

/* Ищет в полуинтервале [low, high). Инвариант: все записи левее low
   раньше moment, все записи начиная с high — не раньше. */
static long lower_bound(const LogRecord *records, long low, long high, Timestamp moment) {
    /* Диапазон пуст — граница найдена. Без этого условия
       поиск зацикливается на двух элементах. */
    if (low >= high) {
        return low;
    }
    /* Середина без переполнения: (low + high) / 2 переполнилось бы
       на больших номерах. */
    long middle = low + (high - low) / 2;
    if (records[middle].time < moment) {
        return lower_bound(records, middle + 1, high, moment);
    }
    /* middle может оказаться ответом, поэтому он остаётся в диапазоне. */
    return lower_bound(records, low, middle, moment);
}

long search_not_before(const LogFile *log, Timestamp moment) {
    return lower_bound(log->records, 0, log->count, moment);
}

int sort_records_for_cost(LogRecord *records, long count) {
    if (!reserve_buffer(count)) {
        return 0;
    }
    int depth = 0;
    merge_sort(records, 0, count, 1, &depth);
    return 1;
}
