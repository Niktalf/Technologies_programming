#ifndef TIMESTAMP_H
#define TIMESTAMP_H

#include <stdint.h>

/* Практика 2: время записи, упакованное в одно число.
   Порядок чисел совпадает с порядком моментов времени, поэтому записи
   можно сравнивать и сортировать целочисленным сравнением. */

typedef int64_t Timestamp;

#define TIMESTAMP_INVALID ((Timestamp)-1)

/* Упаковывает дату и время. Возвращает TIMESTAMP_INVALID при бессмысленных значениях. */
Timestamp timestamp_pack(int year, int month, int day, int hour, int minute, int second);

/* Обратное преобразование: нужно для вывода. Все указатели обязательны. */
void timestamp_unpack(Timestamp value, int *year, int *month, int *day,
                      int *hour, int *minute, int *second);

/* Час суток: понадобится на практике 3 для распределения по часам. */
int timestamp_hour(Timestamp value);

#endif /* TIMESTAMP_H */
