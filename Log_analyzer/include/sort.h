#ifndef SORT_H
#define SORT_H

#include "logfile.h"

int sort_by_time(LogFile *log);
long search_not_before(const LogFile *log, Timestamp moment);

#endif // SORT_H
