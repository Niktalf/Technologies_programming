#include <stddef.h>
#include <stdio.h>

#include "timestamp.h"

/* Каждое поле умножается на диапазон всех младших полей.
   Такая упаковка не считает дни в году, зато сохраняет порядок. */
#define MONTHS_IN_YEAR  12
#define DAYS_IN_MONTH   31
#define HOURS_IN_DAY    24
#define MINUTES_IN_HOUR 60
#define SECONDS_IN_MIN  60

Timestamp timestamp_pack(const DateTime *m) {
    if (m == NULL || m->year < 0 || m->month < 1 || m->month > 12 || m->day < 1 || m->day > 31
        || m->hour < 0 || m->hour > 23 || m->minute < 0 || m->minute > 59
        || m->second < 0 || m->second > 59) {
        return TIMESTAMP_INVALID;
    }
    Timestamp value = m->year;
    value = value * MONTHS_IN_YEAR + (m->month - 1);
    value = value * DAYS_IN_MONTH + (m->day - 1);
    value = value * HOURS_IN_DAY + m->hour;
    value = value * MINUTES_IN_HOUR + m->minute;
    value = value * SECONDS_IN_MIN + m->second;
    return value;
}

/* Распаковка идёт в обратном порядке: от младшего поля к старшему.
   Остаток от деления даёт поле, частное — всё, что старше него.
   Структура возвращается по значению: она маленькая, шесть чисел. */
DateTime timestamp_unpack(Timestamp value) {
    DateTime m = { 0, 0, 0, 0, 0, 0 };
    if (value < 0) {
        return m;
    }
    m.second = (int)(value % SECONDS_IN_MIN);
    value /= SECONDS_IN_MIN;
    m.minute = (int)(value % MINUTES_IN_HOUR);
    value /= MINUTES_IN_HOUR;
    m.hour = (int)(value % HOURS_IN_DAY);
    value /= HOURS_IN_DAY;
    m.day = (int)(value % DAYS_IN_MONTH) + 1;
    value /= DAYS_IN_MONTH;
    m.month = (int)(value % MONTHS_IN_YEAR) + 1;
    value /= MONTHS_IN_YEAR;
    m.year = (int)value;
    return m;
}

Timestamp timestamp_parse(const char *date, const char *time) {
    if (date == NULL || time == NULL) {
        return TIMESTAMP_INVALID;
    }
    DateTime m;
    if (sscanf(date, "%d-%d-%d", &m.year, &m.month, &m.day) != 3
        || sscanf(time, "%d:%d:%d", &m.hour, &m.minute, &m.second) != 3) {
        return TIMESTAMP_INVALID;
    }
    return timestamp_pack(&m);
}

void timestamp_print(Timestamp value) {
    DateTime m = timestamp_unpack(value);
    printf("%04d-%02d-%02d %02d:%02d:%02d", m.year, m.month, m.day, m.hour, m.minute, m.second);
}

int timestamp_hour(Timestamp value) {
    if (value < 0) {
        return -1;
    }
    return (int)((value / (SECONDS_IN_MIN * MINUTES_IN_HOUR)) % HOURS_IN_DAY);
}
