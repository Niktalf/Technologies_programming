#include <string.h>

#include "range.h"

typedef struct {
    const char *key;
    const char *value;
} Pair;

static Pair pairs[STORE_CAPACITY];
static Pair buffer[STORE_CAPACITY];
static int  pair_count;

static void collect(const char *key, const char *value, void *context) {
    (void)context;
    pairs[pair_count].key = key;
    pairs[pair_count].value = value;
    ++pair_count;
}

static void merge(const int left, const int middle, const int right) {
    int i = left;
    int j = middle;
    int k = left;

    while (i < middle && j < right) {
        if (strcmp(pairs[i].key, pairs[j].key) <= 0) {
            buffer[k++] = pairs[i++];
        } else {
            buffer[k++] = pairs[j++];
        }
    }
    while (i < middle) {
        buffer[k++] = pairs[i++];
    }
    while (j < right) {
        buffer[k++] = pairs[j++];
    }
    for (k = left; k < right; ++k) {
        pairs[k] = buffer[k];
    }
}

static void merge_sort(const int left, const int right, const int depth, int *max_depth) {
    if (depth > *max_depth) {
        *max_depth = depth;
    }
    if (right - left < 2) {
        return;
    }
    const int middle = left + (right - left) / 2;

    merge_sort(left, middle, depth + 1, max_depth);
    merge_sort(middle, right, depth + 1, max_depth);
    merge(left, middle, right);
}

static int lower_bound(const int low, const int high, const char *key) {
    if (low >= high) {
        return low;
    }
    const int middle = low + (high - low) / 2;
    if (strcmp(pairs[middle].key, key) < 0) {
        return lower_bound(middle + 1, high, key);
    }
    return lower_bound(low, middle, key);
}

static int upper_bound(const int low, const int high, const char *key) {
    if (low >= high) {
        return low;
    }

    const int middle = low + (high - low) / 2;
    if (strcmp(pairs[middle].key, key) <= 0) {
        return upper_bound(middle + 1, high, key);
    }

    return upper_bound(low, middle, key);
}

static void prepare(const Store *store, int *depth_out) {
    int depth = 0;

    pair_count = 0;
    store_for_each(store, collect, NULL);
    merge_sort(0, pair_count, 1, &depth);
    if (depth_out != NULL) {
        *depth_out = depth;
    }
}

int range_query(const Store *store, const char *from, const char *to, const RangeVisitor visit, void *context, int *depth_out) {
    if (store == NULL || from == NULL || to == NULL || visit == NULL) {
        return 0;
    }
    prepare(store, depth_out);

    if (strcmp(from, to) > 0) {
        return 0;
    }
    const int first = lower_bound(0, pair_count, from);
    const int last = upper_bound(0, pair_count, to);

    for (int i = first; i < last; ++i) {
        visit(pairs[i].key, pairs[i].value, context);
    }
    return last - first;
}

int range_all(const Store *store, const RangeVisitor visit, void *context, int *depth_out) {
    if (store == NULL || visit == NULL) {
        return 0;
    }
    prepare(store, depth_out);
    for (int i = 0; i < pair_count; ++i) {
        visit(pairs[i].key, pairs[i].value, context);
    }
    return pair_count;
}
