#include <stddef.h>

#include "timestamp.h"

/* Каждое поле умножается на диапазон всех младших полей.
   Такая упаковка не считает дни в году, зато сохраняет порядок,
   а больше от неё на практике 2 ничего не требуется. */
#define MONTHS_IN_YEAR  12
#define DAYS_IN_MONTH   31
#define HOURS_IN_DAY    24
#define MINUTES_IN_HOUR 60
#define SECONDS_IN_MIN  60

Timestamp timestamp_pack(int year, int month, int day, int hour, int minute, int second) {
    if (year < 0 || month < 1 || month > 12 || day < 1 || day > 31
        || hour < 0 || hour > 23 || minute < 0 || minute > 59
        || second < 0 || second > 59) {
        return TIMESTAMP_INVALID;
    }

    Timestamp value = year;
    value = value * MONTHS_IN_YEAR + (month - 1);
    value = value * DAYS_IN_MONTH + (day - 1);
    value = value * HOURS_IN_DAY + hour;
    value = value * MINUTES_IN_HOUR + minute;
    value = value * SECONDS_IN_MIN + second;
    return value;
}

/* Распаковка идёт в обратном порядке: от младшего поля к старшему.
   Остаток от деления даёт поле, частное — всё, что старше него. */
void timestamp_unpack(Timestamp value, int *year, int *month, int *day,
                      int *hour, int *minute, int *second) {
    if (value < 0 || year == NULL || month == NULL || day == NULL
        || hour == NULL || minute == NULL || second == NULL) {
        return;
    }
    *second = (int)(value % SECONDS_IN_MIN);
    value /= SECONDS_IN_MIN;
    *minute = (int)(value % MINUTES_IN_HOUR);
    value /= MINUTES_IN_HOUR;
    *hour = (int)(value % HOURS_IN_DAY);
    value /= HOURS_IN_DAY;
    *day = (int)(value % DAYS_IN_MONTH) + 1;
    value /= DAYS_IN_MONTH;
    *month = (int)(value % MONTHS_IN_YEAR) + 1;
    value /= MONTHS_IN_YEAR;
    *year = (int)value;
}

int timestamp_hour(Timestamp value) {
    if (value < 0) {
        return -1;
    }
    return (int)((value / (SECONDS_IN_MIN * MINUTES_IN_HOUR)) % HOURS_IN_DAY);
}
