#ifndef QUERY_H
#define QUERY_H

#include "level.h"
#include "logfile.h"

/* Практика 6: выборки из журнала. Практика 8: вынесены из оболочки
   в отдельный модуль запросов.

   Каждая выборка печатает первые LOGFILE_SHOW подходящих записей
   и возвращает, сколько подходящих записей всего. Все выборки учитывают
   фильтр уровней mask, заданный командой level. */

/* Записи, текст которых содержит word без учёта регистра. */
long query_text(const LogFile *log, LevelMask mask, const char *word);

/* Записи модуля module без учёта регистра. */
long query_module(const LogFile *log, LevelMask mask, const char *module);

/* Записи с временем от from до to включительно. */
long query_interval(const LogFile *log, LevelMask mask, Timestamp from, Timestamp to);

#endif /* QUERY_H */
