#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "binlog.h"

/* Пишет число size байтами, младшим вперёд. Возвращает 1 при успехе. */
static int write_number(FILE *file, unsigned long long value, int size) {
    unsigned char bytes[8];
    for (int i = 0; i < size; ++i) {
        bytes[i] = (unsigned char)((value >> (8 * i)) & 0xFFu);
    }
    return fwrite(bytes, 1, (size_t)size, file) == (size_t)size;
}

/* Читает число из size байтов, младшим вперёд. Возвращает 0, если файл кончился. */
static int read_number(FILE *file, unsigned long long *out, int size) {
    unsigned char bytes[8];
    if (fread(bytes, 1, (size_t)size, file) != (size_t)size) {
        return 0;
    }
    *out = 0;
    for (int i = 0; i < size; ++i) {
        *out |= (unsigned long long)bytes[i] << (8 * i);
    }
    return 1;
}

/* Пишет одну запись. Возвращает 1 при успехе. */
static int write_record(FILE *file, const LogRecord *r) {
    size_t module_length = strlen(r->module);
    size_t text_length = r->text != NULL ? strlen(r->text) : 0;
    return write_number(file, (unsigned long long)r->time, 8)
           && write_number(file, (unsigned long long)r->level, 1)
           && write_number(file, module_length, 1)
           && fwrite(r->module, 1, module_length, file) == module_length
           && write_number(file, text_length, 4)
           && (text_length == 0 || fwrite(r->text, 1, text_length, file) == text_length);
}

LogStatus binlog_save(const LogFile *log, const char *path) {
    if (log == NULL || path == NULL) {
        return LOG_ERR_ARG;
    }
    /* Двоичный режим обязателен: в текстовом байты, совпадающие с кодом
       перевода строки, на части систем портятся. */
    FILE *file = fopen(path, "wb");
    if (file == NULL) {
        return LOG_ERR_OPEN;
    }
    int ok = fwrite(BINLOG_SIGNATURE, 1, 4, file) == 4
             && write_number(file, BINLOG_VERSION, 2)
             && write_number(file, (unsigned long long)log->count, 8);
    for (long i = 0; i < log->count && ok; ++i) {
        ok = write_record(file, &log->records[i]);
    }
    /* Часть ошибок записи видна только при закрытии файла. */
    if (fclose(file) != 0 || !ok) {
        return LOG_ERR_WRITE;
    }
    return LOG_OK;
}

/* Читает и проверяет заголовок: сигнатуру, версию и число записей. */
static LogStatus read_header(FILE *file, unsigned long long *out_count) {
    char signature[4];
    if (fread(signature, 1, 4, file) != 4 || memcmp(signature, BINLOG_SIGNATURE, 4) != 0) {
        return LOG_ERR_FORMAT; /* чужой файл отвергается сразу */
    }
    unsigned long long version = 0;
    if (!read_number(file, &version, 2)) {
        return LOG_ERR_CORRUPT;
    }
    if (version != BINLOG_VERSION) {
        return LOG_ERR_FORMAT;
    }
    if (!read_number(file, out_count, 8)) {
        return LOG_ERR_CORRUPT;
    }
    /* Число записей проверяется до того, как под него что-то выделяется. */
    if (*out_count > (unsigned long long)BINLOG_COUNT_LIMIT) {
        return LOG_ERR_CORRUPT;
    }
    return LOG_OK;
}

/* Читает одну запись. При успехе record->text выделен в куче;
   при ошибке выделенного не остаётся. */
static LogStatus read_record(FILE *file, LogRecord *record) {
    unsigned long long time_value = 0;
    unsigned long long level_value = 0;
    unsigned long long module_length = 0;
    if (!read_number(file, &time_value, 8) || !read_number(file, &level_value, 1)
        || !read_number(file, &module_length, 1)) {
        return LOG_ERR_CORRUPT;
    }
    /* Длине из файла не доверяем: проверяем до чтения по ней. */
    if (level_value >= LEVEL_COUNT || module_length == 0
        || module_length >= LOGFILE_MODULE_MAX) {
        return LOG_ERR_CORRUPT;
    }
    if (fread(record->module, 1, (size_t)module_length, file) != module_length) {
        return LOG_ERR_CORRUPT;
    }
    record->module[module_length] = '\0';
    record->time = (Timestamp)time_value;
    record->level = (Level)level_value;

    unsigned long long text_length = 0;
    if (!read_number(file, &text_length, 4)) {
        return LOG_ERR_CORRUPT;
    }
    if (text_length > BINLOG_TEXT_LIMIT) {
        return LOG_ERR_CORRUPT;
    }
    record->text = malloc((size_t)text_length + 1);
    if (record->text == NULL) {
        return LOG_ERR_MEMORY;
    }
    if (fread(record->text, 1, (size_t)text_length, file) != text_length) {
        free(record->text);
        record->text = NULL;
        return LOG_ERR_CORRUPT;
    }
    record->text[text_length] = '\0';
    return LOG_OK;
}

/* Читает все записи во временный журнал loaded. */
static LogStatus read_records(FILE *file, unsigned long long count, LogFile *loaded) {
    LogStatus status = LOG_OK;
    for (unsigned long long i = 0; i < count && status == LOG_OK; ++i) {
        LogRecord record;
        status = read_record(file, &record);
        if (status == LOG_OK) {
            status = logfile_append(loaded, &record);
            if (status != LOG_OK) {
                free(record.text); /* журнал текст не получил — освобождаем сами */
            }
        }
    }
    return status;
}

LogStatus binlog_load(LogFile *log, const char *path) {
    if (log == NULL || path == NULL) {
        return LOG_ERR_ARG;
    }
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        return LOG_ERR_OPEN;
    }

    /* Читаем во временный журнал: при любой ошибке прежний остаётся. */
    LogFile loaded;
    logfile_init(&loaded);
    unsigned long long count = 0;
    LogStatus status = read_header(file, &count);
    if (status == LOG_OK) {
        status = read_records(file, count, &loaded);
    }
    fclose(file); /* файл открыт на чтение: потерять при закрытии нечего */

    if (status != LOG_OK) {
        logfile_destroy(&loaded); /* всё, что успели прочитать, освобождаем */
        return status;
    }
    loaded.lines = (long)count;
    loaded.loaded = 1;
    strncpy(loaded.path, path, sizeof loaded.path - 1);
    loaded.path[sizeof loaded.path - 1] = '\0';

    logfile_destroy(log);
    *log = loaded;
    return LOG_OK;
}
