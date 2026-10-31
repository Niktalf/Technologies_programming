#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "logfile.h"
#include "parse.h"

void logfile_init(LogFile *log) {
    if (log == NULL) {
        return;
    }
    log->records = NULL;
    log->capacity = 0;
    log->count = 0;
    log->lines = 0;
    log->broken = 0;
    log->no_memory = 0;
    log->truncated = 0;
    log->loaded = 0;
    log->sorted = 0;
    log->path[0] = '\0';
}

void logfile_destroy(LogFile *log) {
    if (log == NULL || log->records == NULL) {
        return;
    }
    /* Сначала текст каждой записи, потом сам массив: в обратном порядке
       адреса текстов были бы уже потеряны. */
    for (long i = 0; i < log->count; ++i) {
        free(log->records[i].text);
    }
    free(log->records);
    log->records = NULL;
    log->capacity = 0;
    log->count = 0;
    log->loaded = 0;
}

/* Удваивает вместимость. Возвращает 1 при успехе. */
static int grow(LogFile *log) {
    long new_capacity = log->capacity > 0 ? log->capacity * 2 : LOGFILE_FIRST_CAPACITY;
    /* Результат перевыделения кладётся во временный указатель: при неудаче
       старый блок ещё нужен, чтобы его освободить. */
    LogRecord *bigger = realloc(log->records, (size_t)new_capacity * sizeof(LogRecord));
    if (bigger == NULL) {
        return 0;
    }
    log->records = bigger;
    log->capacity = new_capacity;
    return 1;
}

LogStatus logfile_append(LogFile *log, const LogRecord *record) {
    if (log == NULL || record == NULL) {
        return LOG_ERR_ARG;
    }
    if (log->count >= log->capacity && !grow(log)) {
        return LOG_ERR_MEMORY;
    }
    log->records[log->count] = *record;
    ++log->count;
    return LOG_OK;
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
    LogStatus status = parse_line(line, &record);
    if (status == LOG_ERR_FORMAT) {
        ++log->broken;
        return;
    }
    if (status != LOG_OK) {
        ++log->no_memory; /* разбор не смог выделить память под текст */
        return;
    }
    /* Массив растёт сам; если памяти не хватило, текст записи
       освобождается здесь — журнал его так и не получил. */
    if (logfile_append(log, &record) != LOG_OK) {
        free(record.text);
        ++log->no_memory;
    }
}

LogStatus logfile_load(LogFile *log, const char *path) {
    if (log == NULL || path == NULL) {
        return LOG_ERR_ARG;
    }
    FILE *file = fopen(path, "r");
    if (file == NULL) {
        return LOG_ERR_OPEN;
    }

    logfile_destroy(log);
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

    /* Ошибка чтения посреди файла выглядит для fgets как конец файла.
       Отличить их можно только через ferror: иначе оборванное чтение
       сошло бы за короткий журнал. */
    int read_failed = ferror(file);
    fclose(file); /* файл открыт на чтение: данных, которые fclose мог бы потерять, нет */
    if (read_failed) {
        return LOG_ERR_CORRUPT;
    }
    log->loaded = 1;
    return LOG_OK;
}

void logfile_print_record(const LogRecord *record) {
    timestamp_print(record->time);
    printf("  %-5s  %-8s  %s\n", level_name(record->level), record->module, record->text);
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
