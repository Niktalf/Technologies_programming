#pragma once

#include <string>
#include <utility>
#include <vector>

#include "log_book.hpp"

// Практика 14: аналитические запросы к журналу — filter, map, reduce.
//
// Каждый запрос — комбинация трёх операций стандартной библиотеки:
//   filter — select (практика 13) или std::count_if: отобрать записи;
//   map    — std::transform: каждую запись превратить в значение;
//   reduce — std::accumulate: свернуть значения в одно.
// Плюс группировка словарём там, где нужно «сколько у каждого».
// Ни один запрос не написан циклом с переменной-накопителем.

// Доля записей уровней ERROR и FATAL от всех, от 0 до 1. Пустой журнал — 0.
double error_share(const LogBook& book);

// Модули с наибольшим числом записей, самые частые первыми; при равенстве —
// по алфавиту. Не больше limit штук.
std::vector<std::pair<std::string, long>> noisiest_modules(const LogBook& book,
                                                           std::size_t limit);

// Средний интервал между соседними по времени ошибками (ERROR и FATAL),
// в секундах. Интервалов на один меньше, чем ошибок: при нуле и одной
// ошибке интервалов нет — тогда false, и деления на ноль не будет.
bool mean_error_interval(const LogBook& book, double& seconds);

// Час суток с наибольшим числом записей и это число. При равенстве —
// более ранний час. Пустой журнал — false.
bool busiest_hour(const LogBook& book, int& hour, long& records);

// Сколько различных модулей. Не зависит от порядка записей.
std::size_t distinct_modules(const LogBook& book);
