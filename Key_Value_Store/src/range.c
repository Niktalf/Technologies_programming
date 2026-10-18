#include <stdlib.h>
#include <string.h>

#include "range.h"

typedef struct {
    const char *key;
    const char *value;
} Pair;

/* Всё, что нужно одному запросу, лежит в одной структуре и живёт
   ровно столько, сколько длится запрос. Глобальных массивов нет:
   два хранилища, опрашиваемые по очереди, друг другу не мешают.

   Указатели на ключи и значения смотрят внутрь ячеек таблицы. Они
   действительны, пока таблицу не меняют, а во время запроса её никто
   не меняет. */
typedef struct {
    Pair *pairs;
    Pair *buffer; /* временный массив для слияния */
    int count;
    int capacity;
} Sorted;

static void collect(const char *key, const char *value, void *context) {
    Sorted *sorted = context;
    /* Число пар известно заранее, но лишняя проверка дешевле порчи памяти. */
    if (sorted->count >= sorted->capacity) {
        return;
    }
    sorted->pairs[sorted->count].key = key;
    sorted->pairs[sorted->count].value = value;
    ++sorted->count;
}

/* Сливает две упорядоченные половины [left, middle) и [middle, right)
   через временный массив. */
static void merge(Sorted *s, int left, int middle, int right) {
    int i = left;
    int j = middle;
    int k = left;
    while (i < middle && j < right) {
        if (strcmp(s->pairs[i].key, s->pairs[j].key) <= 0) {
            s->buffer[k] = s->pairs[i];
            ++i;
        } else {
            s->buffer[k] = s->pairs[j];
            ++j;
        }
        ++k;
    }
    /* Одна из половин кончилась; остаток другой уже упорядочен. */
    while (i < middle) {
        s->buffer[k] = s->pairs[i];
        ++i;
        ++k;
    }
    while (j < right) {
        s->buffer[k] = s->pairs[j];
        ++j;
        ++k;
    }
    for (int n = left; n < right; ++n) {
        s->pairs[n] = s->buffer[n];
    }
}

static void merge_sort(Sorted *s, int left, int right, int depth, int *max_depth) {
    if (depth > *max_depth) {
        *max_depth = depth;
    }
    if (right - left < 2) {
        return; /* ноль или один элемент уже упорядочены */
    }
    int middle = left + (right - left) / 2;
    merge_sort(s, left, middle, depth + 1, max_depth);
    merge_sort(s, middle, right, depth + 1, max_depth);
    merge(s, left, middle, right);
}

/* Первый номер в [low, high), ключ которого не меньше key. */
static int lower_bound(const Sorted *s, int low, int high, const char *key) {
    if (low >= high) {
        return low;
    }
    int middle = low + (high - low) / 2;
    if (strcmp(s->pairs[middle].key, key) < 0) {
        return lower_bound(s, middle + 1, high, key);
    }
    return lower_bound(s, low, middle, key);
}

/* Первый номер в [low, high), ключ которого строго больше key.
   Разница с lower_bound — одно сравнение, и она решает, войдёт ли
   правая граница в диапазон. */
static int upper_bound(const Sorted *s, int low, int high, const char *key) {
    if (low >= high) {
        return low;
    }
    int middle = low + (high - low) / 2;
    if (strcmp(s->pairs[middle].key, key) <= 0) {
        return upper_bound(s, middle + 1, high, key);
    }
    return upper_bound(s, low, middle, key);
}

/* Освобождает оба массива. free(NULL) ничего не делает, поэтому функцию
   можно звать и тогда, когда выделился только один из них. */
static void sorted_free(Sorted *s) {
    free(s->pairs);
    free(s->buffer);
    s->pairs = NULL;
    s->buffer = NULL;
    s->count = 0;
    s->capacity = 0;
}

/* Собирает пары и сортирует их. Возвращает 1 при успехе.
   При неудаче ничего выделенного не остаётся. */
static int prepare(const Store *store, Sorted *s, int *depth_out) {
    int total = store_count(store);
    s->count = 0;
    s->capacity = total;
    /* +1: malloc от нуля байт вправе вернуть NULL, а пустое хранилище — не ошибка. */
    s->pairs = malloc((size_t)(total + 1) * sizeof(Pair));
    s->buffer = malloc((size_t)(total + 1) * sizeof(Pair));
    if (s->pairs == NULL || s->buffer == NULL) {
        sorted_free(s);
        return 0;
    }
    store_for_each(store, collect, s);

    int depth = 0;
    merge_sort(s, 0, s->count, 1, &depth);
    if (depth_out != NULL) {
        *depth_out = depth;
    }
    return 1;
}

int range_query(const Store *store, const char *from, const char *to,
                RangeVisitor visit, void *context, int *depth_out) {
    if (store == NULL || from == NULL || to == NULL || visit == NULL) {
        return -1;
    }
    /* Начальная граница больше конечной — диапазон пуст, сортировать незачем. */
    if (strcmp(from, to) > 0) {
        return 0;
    }
    Sorted s;
    if (!prepare(store, &s, depth_out)) {
        return -1;
    }
    int first = lower_bound(&s, 0, s.count, from);
    int last = upper_bound(&s, 0, s.count, to);
    for (int i = first; i < last; ++i) {
        visit(s.pairs[i].key, s.pairs[i].value, context);
    }
    sorted_free(&s);
    return last - first;
}

int range_all(const Store *store, RangeVisitor visit, void *context, int *depth_out) {
    if (store == NULL || visit == NULL) {
        return -1;
    }
    Sorted s;
    if (!prepare(store, &s, depth_out)) {
        return -1;
    }
    for (int i = 0; i < s.count; ++i) {
        visit(s.pairs[i].key, s.pairs[i].value, context);
    }
    int found = s.count;
    sorted_free(&s);
    return found;
}
