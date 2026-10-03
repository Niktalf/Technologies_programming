#ifndef TIMESTAMP_H
#define TIMESTAMP_H

#include <stdint.h>

typedef int64_t Timestamp;

#define TIMESTAMP_INVALID ((Timestamp)-1)

Timestamp timestamp_pack(int year, int month, int day, int hour, int minute, int second);
void timestamp_unpack(Timestamp value, int *year, int *month, int *day,
                      int *hour, int *minute, int *second);
int timestamp_hour(Timestamp value);

#endif // TIMESTAMP_H
