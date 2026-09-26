#ifndef LOGFILE_H
#define LOGFILE_H

#include <stddef.h>

#define LOGFILE_TAIL_SIZE 10
#define LOGFILE_LINE_MAX  512
#define LOGFILE_HEAD_SIZE 10

typedef enum {
    LOG_OK = 0,
    LOG_ERR_ARG,
    LOG_ERR_OPEN
} LogStatus;

typedef struct {
    char   head[LOGFILE_HEAD_SIZE][LOGFILE_LINE_MAX];
    char   tail[LOGFILE_TAIL_SIZE][LOGFILE_LINE_MAX];
    long   total;
    long   truncated;
    int    loaded;
} LogFile;

void logfile_init(LogFile *log);

LogStatus logfile_scan(LogFile *log, const char *path);

const char *logfile_status_text(LogStatus status);

void logfile_print_head(const LogFile *log, int count);
void logfile_print_tail(const LogFile *log, int count);

#endif // LOGFILE_H
