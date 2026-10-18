#ifndef LOGFILE_H
#define LOGFILE_H

#include "level.h"
#include "timestamp.h"

/* Практика 3: журнал хранится в памяти массивом разобранных записей,
   а не перечитывается на каждую команду. Размер массива фиксирован,
   динамический массив появится на практике 7. */

#define LOGFILE_LINE_MAX   512   /* длина строки вместе с переводом строки и нулём */
#define LOGFILE_CAPACITY   10000 /* сколько записей помещается в массив */
#define LOGFILE_MODULE_MAX 32
#define LOGFILE_TEXT_MAX   256
#define LOGFILE_PATH_MAX   256
#define LOGFILE_SHOW       10    /* сколько записей печатают head и tail */

/* Одна запись журнала: строка «время|уровень|модуль|текст», разобранная на поля. */
typedef struct {
    Timestamp time;
    Level level;
    char module[LOGFILE_MODULE_MAX];
    char text[LOGFILE_TEXT_MAX];
} LogRecord;

typedef enum {
    LOG_OK = 0,
    LOG_ERR_ARG,
    LOG_ERR_OPEN
} LogStatus;

typedef struct {
    LogRecord records[LOGFILE_CAPACITY];
    long count;     /* сколько записей в массиве */
    long lines;     /* сколько строк в файле */
    long broken;    /* строк, которые не удалось разобрать */
    long overflow;  /* записей, не поместившихся в массив */
    long truncated; /* строк длиннее буфера чтения */
    int loaded;     /* признак загруженного журнала */
    char path[LOGFILE_PATH_MAX];
} LogFile;

/* Объект журнала весит около трёх мегабайт, поэтому он статический.
   Локальной переменной в функции его делать нельзя: стек меньше. */

void logfile_init(LogFile *log);

/* Читает файл целиком в массив записей. Строки, которые не удалось
   разобрать, считаются, но не прерывают чтение. */
LogStatus logfile_load(LogFile *log, const char *path);

const char *logfile_status_text(LogStatus status);

void logfile_print_record(const LogRecord *record);
/* Первые и последние count записей среди тех, что проходят фильтр mask.
   Фильтр по уровням, введённый на практике 2, действует на все команды вывода. */
void logfile_print_head(const LogFile *log, int count, LevelMask mask);
void logfile_print_tail(const LogFile *log, int count, LevelMask mask);

/* Сколько записей проходит фильтр. */
long logfile_count_matching(const LogFile *log, LevelMask mask);

#endif /* LOGFILE_H */
