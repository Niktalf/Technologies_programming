#include <stdio.h>
#include <string.h>

#include "logfile.h"
#include "parse.h"

void logfile_init(LogFile *log)
{
    if (log == NULL) {
        return;
    }
    log->count = 0;
    log->lines = 0;
    log->broken = 0;
    log->overflow = 0;
    log->truncated = 0;
    log->loaded = 0;
    log->sorted = 0;
    log->path[0] = '\0';
}

const char *logfile_status_text(const LogStatus status) {
    switch (status) {
    case LOG_OK:       return "success";
    case LOG_ERR_ARG:  return "invalid argument";
    case LOG_ERR_OPEN: return "failed to open file";
    default:           return "unknown error";
    }
}

static void strip_line_end(char *line) {
    size_t length = strlen(line);
    while (length > 0 && (line[length - 1] == '\n' || line[length - 1] == '\r')) {
        line[--length] = '\0';
    }
}

LogStatus logfile_load(LogFile *log, const char *path) {
    char line[LOGFILE_LINE_MAX];

    if (log == NULL || path == NULL) {
        return LOG_ERR_ARG;
    }
    FILE *file = fopen(path, "r");
    if (file == NULL) {
        return LOG_ERR_OPEN;
    }

    logfile_init(log);
    strncpy(log->path, path, sizeof log->path - 1);
    log->path[sizeof log->path - 1] = '\0';

    while (fgets(line, (int)sizeof line, file) != NULL) {
        LogRecord record;

        if (strchr(line, '\n') == NULL && !feof(file)) {
            int c;
            while ((c = fgetc(file)) != EOF && c != '\n') {}
            ++log->truncated;
        }
        strip_line_end(line);
        ++log->lines;

        if (line[0] == '\0') {
            continue;
        }
        if (!parse_line(line, &record)) {
            ++log->broken;
            continue;
        }
        if (log->count >= LOGFILE_CAPACITY) {
            ++log->overflow;
            continue;
        }
        log->records[log->count++] = record;
    }

    fclose(file);
    log->loaded = 1;
    return LOG_OK;
}

int logfile_time_range(const LogFile *log, Timestamp *out_first, Timestamp *out_last) {
    if (log == NULL || out_first == NULL || out_last == NULL || log->count == 0) {
        return 0;
    }
    Timestamp first = log->records[0].time;
    Timestamp last = first;
    for (long i = 1; i < log->count; ++i) {
        const Timestamp t = log->records[i].time;

        if (t < first) {
            first = t;
        }
        if (t > last) {
            last = t;
        }
    }
    *out_first = first;
    *out_last = last;
    return 1;
}

void logfile_print_record(const LogRecord *record) {
    int y, mo, d, h, mi, s;

    timestamp_unpack(record->time, &y, &mo, &d, &h, &mi, &s);
    printf("%04d-%02d-%02d %02d:%02d:%02d  %-5s  %-8s  %s\n",
           y, mo, d, h, mi, s, level_name(record->level), record->module, record->text);
}

void logfile_print_head(const LogFile *log, const int count) {
    const long limit = count < log->count ? count : log->count;
    for (long i = 0; i < limit; ++i) {
        logfile_print_record(&log->records[i]);
    }
}

void logfile_print_tail(const LogFile *log, const int count){
    const long first = log->count > count ? log->count - count : 0;
    for (long i = first; i < log->count; ++i) {
        logfile_print_record(&log->records[i]);
    }
}
