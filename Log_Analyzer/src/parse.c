#include <stdio.h>
#include <string.h>

#include "parse.h"

/* Копирует кусок строки [begin, end) в буфер с завершающим нулём.
   Возвращает 0, если кусок не помещается. */
static int copy_field(char *destination, size_t size, const char *begin, const char *end) {
    size_t length = (size_t)(end - begin);
    if (length >= size) {
        return 0;
    }
    memcpy(destination, begin, length);
    destination[length] = '\0';
    return 1;
}

/* Разбирает поле времени «ГГГГ-ММ-ДД чч:мм:сс». */
static int parse_time(const char *begin, const char *end, Timestamp *out) {
    char field[64];
    if (!copy_field(field, sizeof field, begin, end)) {
        return 0;
    }
    int year = 0;
    int month = 0;
    int day = 0;
    int hour = 0;
    int minute = 0;
    int second = 0;
    if (sscanf(field, "%d-%d-%d %d:%d:%d", &year, &month, &day, &hour, &minute, &second) != 6) {
        return 0;
    }
    *out = timestamp_pack(year, month, day, hour, minute, second);
    return *out != TIMESTAMP_INVALID;
}

/* Разбирает поле уровня. */
static int parse_level(const char *begin, const char *end, Level *out) {
    char field[64];
    if (!copy_field(field, sizeof field, begin, end)) {
        return 0;
    }
    *out = level_from_word(field);
    return *out != LEVEL_UNKNOWN;
}

int parse_line(const char *line, LogRecord *out) {
    /* Ровно три поиска разделителя, а не разбиение по всем чертам подряд:
       иначе сообщение с чертой внутри потеряет хвост. */
    const char *bar1 = strchr(line, '|');
    if (bar1 == NULL) {
        return 0;
    }
    const char *bar2 = strchr(bar1 + 1, '|');
    if (bar2 == NULL) {
        return 0;
    }
    const char *bar3 = strchr(bar2 + 1, '|');
    if (bar3 == NULL) {
        return 0;
    }

    if (!parse_time(line, bar1, &out->time)) {
        return 0;
    }
    if (!parse_level(bar1 + 1, bar2, &out->level)) {
        return 0;
    }
    /* Модуль: непустой и помещается в поле. */
    if (bar3 == bar2 + 1) {
        return 0;
    }
    if (!copy_field(out->module, sizeof out->module, bar2 + 1, bar3)) {
        return 0;
    }

    /* Текст: всё до конца строки. Слишком длинный обрезается, а не отвергается. */
    size_t text_length = strlen(bar3 + 1);
    if (text_length >= sizeof out->text) {
        text_length = sizeof out->text - 1;
    }
    memcpy(out->text, bar3 + 1, text_length);
    out->text[text_length] = '\0';
    return 1;
}
