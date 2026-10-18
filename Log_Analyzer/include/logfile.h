#ifndef LOGFILE_H
#define LOGFILE_H

#include <stddef.h>

#include "level.h"

/* Журнал на практиках 1–2.
   Весь файл в память не читается: хранятся только счётчики, первые и последние
   строки. Разбор строк на поля появится на практике 6, хранение записей —
   на практике 3.

   Практика 2: первые и последние строки запоминаются с учётом фильтра
   по уровням. Файл целиком в памяти не лежит, поэтому при смене фильтра
   он просматривается заново. */

#define LOGFILE_LINE_MAX  512 /* длина строки вместе с переводом строки и нулём */
#define LOGFILE_HEAD_SIZE 10  /* сколько первых строк запоминается */
#define LOGFILE_TAIL_SIZE 10  /* размер кольцевого буфера последних строк */
#define LOGFILE_PATH_MAX  256

typedef enum {
    LOG_OK = 0,
    LOG_ERR_ARG,
    LOG_ERR_OPEN
} LogStatus;

typedef struct {
    char path[LOGFILE_PATH_MAX];
    char head[LOGFILE_HEAD_SIZE][LOGFILE_LINE_MAX];
    char tail[LOGFILE_TAIL_SIZE][LOGFILE_LINE_MAX];
    long head_number[LOGFILE_HEAD_SIZE]; /* номера этих строк в файле */
    long tail_number[LOGFILE_TAIL_SIZE];
    long total;     /* сколько строк всего */
    long matched;   /* сколько из них проходит фильтр */
    long unknown;   /* сколько строк с неизвестным уровнем */
    long truncated; /* сколько строк не поместилось в буфер и было обрезано */
    LevelMask mask; /* фильтр, с которым сделан последний просмотр */
    int loaded;     /* признак успешного прохода по файлу */
} LogFile;

void logfile_init(LogFile *log);

/* Один проход по файлу: считает строки, запоминает первые и последние
   из тех, что проходят фильтр mask. */
LogStatus logfile_scan(LogFile *log, const char *path, LevelMask mask);

/* Уровень строки журнала вида «время|УРОВЕНЬ|модуль|текст».
   LEVEL_UNKNOWN, если второго поля нет или в нём не уровень. */
Level logfile_line_level(const char *line);

const char *logfile_status_text(LogStatus status);

/* Печать первых и последних строк. Если строк меньше запрошенного, печатает все. */
void logfile_print_head(const LogFile *log, int count);
void logfile_print_tail(const LogFile *log, int count);

#endif /* LOGFILE_H */
