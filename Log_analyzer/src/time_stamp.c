#include <stddef.h>

#include "time_stamp.h"

#define MONTHS_IN_YEAR  12
#define DAYS_IN_MONTH   31
#define HOURS_IN_DAY    24
#define MINUTES_IN_HOUR 60
#define SECONDS_IN_MIN  60

TimeStamp timestamp_pack(const int year, const int month, const int day, const int hour, const int minute, const int second)
{
    if (year < 0 || month < 1 || month > 12 || day < 1 || day > 31
        || hour < 0 || hour > 23 || minute < 0 || minute > 59
        || second < 0 || second > 59) {
        return TIMESTAMP_INVALID;
        }

    TimeStamp value = year;
    value = value * MONTHS_IN_YEAR + (month - 1);
    value = value * DAYS_IN_MONTH  + (day - 1);
    value = value * HOURS_IN_DAY   + hour;
    value = value * MINUTES_IN_HOUR + minute;
    value = value * SECONDS_IN_MIN  + second;
    return value;
}

void timestamp_unpack(TimeStamp value, int *year, int *month, int *day,
                      int *hour, int *minute, int *second)
{
    if (value < 0) {
        return;
    }
    if (second != NULL) { *second = (int)(value % SECONDS_IN_MIN); }
    value /= SECONDS_IN_MIN;
    if (minute != NULL) { *minute = (int)(value % MINUTES_IN_HOUR); }
    value /= MINUTES_IN_HOUR;
    if (hour != NULL)   { *hour = (int)(value % HOURS_IN_DAY); }
    value /= HOURS_IN_DAY;
    if (day != NULL)    { *day = (int)(value % DAYS_IN_MONTH) + 1; }
    value /= DAYS_IN_MONTH;
    if (month != NULL)  { *month = (int)(value % MONTHS_IN_YEAR) + 1; }
    value /= MONTHS_IN_YEAR;
    if (year != NULL)   { *year = (int)value; }
}

int timestamp_hour(const TimeStamp value)
{
    if (value < 0) {
        return -1;
    }
    return (int)(value / (SECONDS_IN_MIN * MINUTES_IN_HOUR) % HOURS_IN_DAY);
}
