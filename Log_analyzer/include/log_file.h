#ifndef LOG_FILE_H
#define LOG_FILE_H

#include "level.h"
#include "time_stamp.h"

#define LOGFILE_LINE_MAX   512
#define LOGFILE_CAPACITY   10000
#define LOGFILE_MODULE_MAX 32
#define LOGFILE_TEXT_MAX   256
#define LOGFILE_PATH_MAX   256
#define LOGFILE_SHOW       10

typedef struct {
    TimeStamp time;
    Level     level;
    char      module[LOGFILE_MODULE_MAX];
    char      text[LOGFILE_TEXT_MAX];
} LogRecord;

typedef enum {
    LOG_OK = 0,
    LOG_ERR_ARG,
    LOG_ERR_OPEN
} LogStatus;

typedef struct {
    LogRecord records[LOGFILE_CAPACITY];
    long count;
    long lines;
    long broken;
    long overflow;
    long truncated;
    int  loaded;
    char path[LOGFILE_PATH_MAX];
} LogFile;

void logfile_init(LogFile *log);

LogStatus logfile_load(LogFile *log, const char *path);

const char *logfile_status_text(LogStatus status);

void logfile_print_record(const LogRecord *record);
void logfile_print_head(const LogFile *log, int count);
void logfile_print_tail(const LogFile *log, int count);

#endif // LOG_FILE_H
