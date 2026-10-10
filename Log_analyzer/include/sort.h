#ifndef SORT_H
#define SORT_H

#include "logfile.h"

int sort_by_time(LogFile *log);
long search_not_before(const LogFile *log, Timestamp moment);
void sort_records_for_cost(LogRecord *records, long count);

#endif // SORT_H
