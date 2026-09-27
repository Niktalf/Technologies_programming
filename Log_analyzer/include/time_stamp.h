#ifndef TIME_STAMP_H
#define TIME_STAMP_H

#include <stdint.h>

typedef int64_t TimeStamp;

#define TIMESTAMP_INVALID ((TimeStamp)-1)

TimeStamp timestamp_pack(int year, int month, int day, int hour, int minute, int second);

void timestamp_unpack(TimeStamp value, int *year, int *month, int *day,
                      int *hour, int *minute, int *second);

int timestamp_hour(TimeStamp value);

#endif // TIME_STAMP_H
