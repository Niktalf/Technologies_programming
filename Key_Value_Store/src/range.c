#include <string.h>

#include "range.h"

typedef struct {
    const char *key;
    const char *value;
} Pair;

/* Указатели на ключи и значения внутри ячеек таблицы. Они действительны,
   пока таблицу не меняют, — а во время запроса её никто не меняет.
   Массивы статические: по 16 тысяч элементов в стек класть не стоит. */
static Pair pairs[STORE_CAPACITY];
static Pair buffer[STORE_CAPACITY];
static int pair_count;

static void collect(const char *key, const char *value, void *context) {
    (void)context; /* контекст здесь не нужен, но так устроен обход */
    pairs[pair_count].key = key;
    pairs[pair_count].value = value;
    ++pair_count;
}

/* Сливает две упорядоченные половины [left, middle) и [middle, right)
   через временный массив. */
static void merge(int left, int middle, int right) {
    int i = left;
    int j = middle;
    int k = left;
    while (i < middle && j < right) {
        if (strcmp(pairs[i].key, pairs[j].key) <= 0) {
            buffer[k] = pairs[i];
            ++i;
        } else {
            buffer[k] = pairs[j];
            ++j;
        }
        ++k;
    }
    /* Одна из половин кончилась; остаток другой уже упорядочен. */
    while (i < middle) {
        buffer[k] = pairs[i];
        ++i;
        ++k;
    }
    while (j < right) {
        buffer[k] = pairs[j];
        ++j;
        ++k;
    }
    for (int n = left; n < right; ++n) {
        pairs[n] = buffer[n];
    }
}

static void merge_sort(int left, int right, int depth, int *max_depth) {
    if (depth > *max_depth) {
        *max_depth = depth;
    }
    /* Условие остановки: ноль или один элемент уже упорядочены. */
    if (right - left < 2) {
        return;
    }
    int middle = left + (right - left) / 2;
    merge_sort(left, middle, depth + 1, max_depth);
    merge_sort(middle, right, depth + 1, max_depth);
    merge(left, middle, right);
}

/* Первый номер в [low, high), ключ которого не меньше key. */
static int lower_bound(int low, int high, const char *key) {
    if (low >= high) {
        return low;
    }
    int middle = low + (high - low) / 2;
    if (strcmp(pairs[middle].key, key) < 0) {
        return lower_bound(middle + 1, high, key);
    }
    return lower_bound(low, middle, key);
}

/* Первый номер в [low, high), ключ которого строго больше key.
   Разница с lower_bound — одно сравнение, и она решает, войдёт ли
   правая граница в диапазон. */
static int upper_bound(int low, int high, const char *key) {
    if (low >= high) {
        return low;
    }
    int middle = low + (high - low) / 2;
    if (strcmp(pairs[middle].key, key) <= 0) {
        return upper_bound(middle + 1, high, key);
    }
    return upper_bound(low, middle, key);
}

/* Собирает все пары и упорядочивает их по ключу. */
static void prepare(const Store *store, int *depth_out) {
    pair_count = 0;
    store_for_each(store, collect, NULL);
    int depth = 0;
    merge_sort(0, pair_count, 1, &depth);
    if (depth_out != NULL) {
        *depth_out = depth;
    }
}

int range_query(const Store *store, const char *from, const char *to,
                RangeVisitor visit, void *context, int *depth_out) {
    if (store == NULL || from == NULL || to == NULL || visit == NULL) {
        return 0;
    }
    prepare(store, depth_out);

    /* Начальная граница больше конечной — диапазон пуст. */
    if (strcmp(from, to) > 0) {
        return 0;
    }
    int first = lower_bound(0, pair_count, from);
    int last = upper_bound(0, pair_count, to);
    for (int i = first; i < last; ++i) {
        visit(pairs[i].key, pairs[i].value, context);
    }
    return last - first;
}

int range_all(const Store *store, RangeVisitor visit, void *context, int *depth_out) {
    if (store == NULL || visit == NULL) {
        return 0;
    }
    prepare(store, depth_out);
    for (int i = 0; i < pair_count; ++i) {
        visit(pairs[i].key, pairs[i].value, context);
    }
    return pair_count;
}
