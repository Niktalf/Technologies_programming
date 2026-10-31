#ifndef TIMESTAMP_H
#define TIMESTAMP_H

#include <stdint.h>

/* Практика 2: время записи, упакованное в одно число.
   Порядок чисел совпадает с порядком моментов времени, поэтому записи
   можно сравнивать и сортировать целочисленным сравнением.

   Практика 8: шесть полей даты и времени собраны в структуру DateTime.
   Раньше функции принимали их шестью отдельными параметрами, а распаковка
   возвращала через шесть указателей; теперь передаётся одна структура. */

typedef int64_t Timestamp;

#define TIMESTAMP_INVALID ((Timestamp)-1)

typedef struct {
    int year;
    int month;  /* 1..12 */
    int day;    /* 1..31 */
    int hour;   /* 0..23 */
    int minute; /* 0..59 */
    int second; /* 0..59 */
} DateTime;

/* Упаковывает дату и время. Возвращает TIMESTAMP_INVALID при бессмысленных значениях. */
Timestamp timestamp_pack(const DateTime *moment);

/* Обратное преобразование: нужно для вывода. */
DateTime timestamp_unpack(Timestamp value);

/* Разбирает «ГГГГ-ММ-ДД» и «чч:мм:сс». Возвращает TIMESTAMP_INVALID,
   если разобрать не удалось или такого момента нет. */
Timestamp timestamp_parse(const char *date, const char *time);

/* Печатает момент в виде «ГГГГ-ММ-ДД чч:мм:сс» без перевода строки. */
void timestamp_print(Timestamp value);

/* Час суток: понадобится на практике 3 для распределения по часам. */
int timestamp_hour(Timestamp value);

#endif /* TIMESTAMP_H */
