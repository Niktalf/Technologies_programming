#include <stdio.h>
#include <string.h>

#include "logfile.h"

void logfile_init(LogFile *log) {
    if (log == NULL) {
        return;
    }
    memset(log, 0, sizeof *log);
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

/* Убирает завершающий перевод строки, если он есть. */
static void strip_newline(char *line) {
    size_t length = strlen(line);
    if (length > 0 && line[length - 1] == '\n') {
        line[length - 1] = '\0';
    }
}

/* Дочитывает и выбрасывает остаток строки до перевода строки или конца файла. */
static void skip_rest_of_line(FILE *file) {
    int c = fgetc(file);
    while (c != '\n' && c != EOF) {
        c = fgetc(file);
    }
}

LogStatus logfile_scan(LogFile *log, const char *path) {
    if (log == NULL || path == NULL) {
        return LOG_ERR_ARG;
    }
    FILE *file = fopen(path, "r");
    if (file == NULL) {
        return LOG_ERR_OPEN;
    }

    logfile_init(log);

    char line[LOGFILE_LINE_MAX];
    while (fgets(line, (int)sizeof line, file) != NULL) {
        /* Строка длиннее буфера: дочитываем хвост, чтобы он не стал новой строкой.
           Последняя строка файла без перевода строки сюда не попадает: после неё
           уже конец файла. */
        if (strchr(line, '\n') == NULL && !feof(file)) {
            skip_rest_of_line(file);
            ++log->truncated;
        }
        strip_newline(line);

        if (log->total < LOGFILE_HEAD_SIZE) {
            strcpy(log->head[log->total], line);
        }
        /* Кольцевой буфер: новая строка занимает место самой старой. */
        strcpy(log->tail[log->total % LOGFILE_TAIL_SIZE], line);
        ++log->total;
    }

    fclose(file);
    log->loaded = 1;
    return LOG_OK;
}

/* Сколько строк печатать: не больше запрошенного, размера буфера и числа строк. */
static long print_limit(const LogFile *log, int count, int buffer_size) {
    long limit = count < buffer_size ? count : buffer_size;
    if (limit > log->total) {
        limit = log->total;
    }
    return limit;
}

void logfile_print_head(const LogFile *log, int count) {
    if (log == NULL || !log->loaded) {
        printf("Log is not loaded.\n");
        return;
    }
    long limit = print_limit(log, count, LOGFILE_HEAD_SIZE);
    for (long i = 0; i < limit; ++i) {
        printf("%ld: %s\n", i + 1, log->head[i]);
    }
}

void logfile_print_tail(const LogFile *log, int count) {
    if (log == NULL || !log->loaded) {
        printf("Log is not loaded.\n");
        return;
    }
    long limit = print_limit(log, count, LOGFILE_TAIL_SIZE);
    for (long i = log->total - limit; i < log->total; ++i) {
        /* Тот же остаток от деления, что при записи, даёт верный порядок. */
        printf("%ld: %s\n", i + 1, log->tail[i % LOGFILE_TAIL_SIZE]);
    }
}
