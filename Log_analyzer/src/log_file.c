#include <stdio.h>
#include <string.h>

#include "log_file.h"

void logfile_init(LogFile *log)
{
    if (log == NULL) {
        return;
    }
    memset(log, 0, sizeof *log);
}

const char *logfile_status_text(LogStatus status)
{
    switch (status) {
    case LOG_OK:       return "success";
    case LOG_ERR_ARG:  return "invalid argument";
    case LOG_ERR_OPEN: return "failed to open file";
    default:           return "unknown error";
    }
}

static void strip_newline(char *line)
{
    const size_t length = strlen(line);

    if (length > 0 && line[length - 1] == '\n') {
        line[length - 1] = '\0';
    }
}

LogStatus logfile_scan(LogFile *log, const char *path)
{
    char line[LOGFILE_LINE_MAX];

    if (log == NULL || path == NULL) {
        return LOG_ERR_ARG;
    }
    FILE *file = fopen(path, "r");
    if (file == NULL) {
        return LOG_ERR_OPEN;
    }

    logfile_init(log);

    while (fgets(line, (int)sizeof line, file) != NULL) {
        if (strchr(line, '\n') == NULL && !feof(file)) {
            int c;
            while ((c = fgetc(file)) != EOF && c != '\n') {}
            ++log->truncated;
        }
        strip_newline(line);

        if (log->total < LOGFILE_HEAD_SIZE) {
            strcpy(log->head[log->total], line);
        }

        strcpy(log->tail[log->total % LOGFILE_TAIL_SIZE], line);
        ++log->total;
    }

    fclose(file);
    log->loaded = 1;
    return LOG_OK;
}

void logfile_print_head(const LogFile *log, int count)
{
    if (log == NULL || !log->loaded) {
        printf("The log is not loaded.\n");
        return;
    }
    long limit = count < LOGFILE_HEAD_SIZE ? count : LOGFILE_HEAD_SIZE;
    if (limit > log->total) {
        limit = log->total;
    }
    for (long i = 0; i < limit; ++i) {
        printf("%ld: %s\n", i + 1, log->head[i]);
    }
}

void logfile_print_tail(const LogFile *log, int count)
{
    if (log == NULL || !log->loaded) {
        printf("The log is not loaded.\n");
        return;
    }
    long limit = count < LOGFILE_TAIL_SIZE ? count : LOGFILE_TAIL_SIZE;
    if (limit > log->total) {
        limit = log->total;
    }
    const long first = log->total - limit;
    for (long i = first; i < log->total; ++i) {
        printf("%ld: %s\n", i + 1, log->tail[i % LOGFILE_TAIL_SIZE]);
    }
}
