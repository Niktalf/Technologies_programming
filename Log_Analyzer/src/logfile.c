#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "logfile.h"
#include "parse.h"

void logfile_init(LogFile *log) {
    if (log == NULL) {
        return;
    }
    /* Массив записей не очищается: значимы только первые count элементов. */
    log->count = 0;
    log->lines = 0;
    log->broken = 0;
    log->overflow = 0;
    log->truncated = 0;
    log->loaded = 0;
    log->sorted = 0;
    log->path[0] = '\0';
}

const char *logfile_status_text(LogStatus status) {
    switch (status) {
    case LOG_OK:
        return "success";
    case LOG_ERR_ARG:
        return "invalid argument";
    case LOG_ERR_OPEN:
        return "cannot open file";
    default:
        return "unknown error";
    }
}

/* Убирает завершающие переводы строки, в том числе виндовый \r. */
static void strip_line_end(char *line) {
    size_t length = strlen(line);
    while (length > 0 && (line[length - 1] == '\n' || line[length - 1] == '\r')) {
        --length;
        line[length] = '\0';
    }
}

/* Дочитывает и выбрасывает остаток строки до перевода строки или конца файла. */
static void skip_rest_of_line(FILE *file) {
    int c = fgetc(file);
    while (c != '\n' && c != EOF) {
        c = fgetc(file);
    }
}

/* Учитывает одну прочитанную строку: пустая пропускается, испорченная
   считается, разобранная кладётся в массив, если есть место. */
static void add_line(LogFile *log, const char *line) {
    ++log->lines;
    if (line[0] == '\0') {
        return; /* пустые строки не считаются испорченными */
    }
    LogRecord record;
    if (!parse_line(line, &record)) {
        ++log->broken;
        return;
    }
    /* Вместимость проверяется до записи в массив, а не после. */
    if (log->count >= LOGFILE_CAPACITY) {
        ++log->overflow;
        return;
    }
    log->records[log->count] = record;
    ++log->count;
}

LogStatus logfile_load(LogFile *log, const char *path) {
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

    char line[LOGFILE_LINE_MAX];
    while (fgets(line, (int)sizeof line, file) != NULL) {
        if (strchr(line, '\n') == NULL && !feof(file)) {
            skip_rest_of_line(file);
            ++log->truncated;
        }
        strip_line_end(line);
        add_line(log, line);
    }

    fclose(file);
    log->loaded = 1;
    return LOG_OK;
}

void logfile_print_record(const LogRecord *record) {
    int year = 0;
    int month = 0;
    int day = 0;
    int hour = 0;
    int minute = 0;
    int second = 0;
    timestamp_unpack(record->time, &year, &month, &day, &hour, &minute, &second);
    printf("%04d-%02d-%02d %02d:%02d:%02d  %-5s  %-8s  %s\n",
           year, month, day, hour, minute, second,
           level_name(record->level), record->module, record->text);
}

void logfile_print_head(const LogFile *log, int count, LevelMask mask) {
    if (log == NULL) {
        return;
    }
    int printed = 0;
    for (long i = 0; i < log->count && printed < count; ++i) {
        if (level_mask_has(mask, log->records[i].level)) {
            logfile_print_record(&log->records[i]);
            ++printed;
        }
    }
}

void logfile_print_tail(const LogFile *log, int count, LevelMask mask) {
    if (log == NULL) {
        return;
    }
    /* Идём с конца, пока не наберём count подходящих записей, —
       так находим, с какой записи начинать. Печатаем уже в прямом порядке. */
    long first = log->count;
    int found = 0;
    while (first > 0 && found < count) {
        --first;
        if (level_mask_has(mask, log->records[first].level)) {
            ++found;
        }
    }
    for (long i = first; i < log->count; ++i) {
        if (level_mask_has(mask, log->records[i].level)) {
            logfile_print_record(&log->records[i]);
        }
    }
}

long logfile_count_matching(const LogFile *log, LevelMask mask) {
    if (log == NULL) {
        return 0;
    }
    long matching = 0;
    for (long i = 0; i < log->count; ++i) {
        if (level_mask_has(mask, log->records[i].level)) {
            ++matching;
        }
    }
    return matching;
}

int logfile_time_range(const LogFile *log, Timestamp *out_first, Timestamp *out_last) {
    if (log == NULL || out_first == NULL || out_last == NULL || log->count == 0) {
        return 0;
    }
    /* Начальные значения берём из первой записи, а не из нуля и не из
       максимума: так проверка работает при любых данных. */
    Timestamp first = log->records[0].time;
    Timestamp last = first;
    for (long i = 1; i < log->count; ++i) {
        Timestamp t = log->records[i].time;
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
