#ifndef VIEW_H
#define VIEW_H

#include "logfile.h"

typedef enum {
    ORDER_TIME = 0,
    ORDER_LEVEL,
    ORDER_MODULE
} Order;

const char *order_name(Order order);
long view_sort(const LogFile *log, Order order);
void view_print(long count_to_print);

const LogRecord *view_at(long index);
long view_count();

#endif // VIEW_H
