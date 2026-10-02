#ifndef RANGE_H
#define RANGE_H

#include "store.h"

typedef void (*RangeVisitor)(const char *key, const char *value, void *context);

int range_query(const Store *store, const char *from, const char *to,
                RangeVisitor visit, void *context, int *depth_out);

int range_all(const Store *store, RangeVisitor visit, void *context, int *depth_out);

#endif // RANGE_H
