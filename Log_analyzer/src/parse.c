#include <stdio.h>
#include <string.h>

#include "parse.h"

static int copy_field(char *destination, const size_t size, const char *begin, const char *end)
{
    const size_t length = (size_t)(end - begin);

    if (length >= size) {
        return 0;
    }
    memcpy(destination, begin, length);
    destination[length] = '\0';
    return 1;
}

int parse_line(const char *line, LogRecord *out)
{
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

    char field[64];
    if (!copy_field(field, sizeof field, line, bar1)) {
        return 0;
    }

    int year, month, day, hour, minute, second;
    if (sscanf(field, "%d-%d-%d %d:%d:%d", &year, &month, &day, &hour, &minute, &second) != 6) {
        return 0;
    }
    out->time = timestamp_pack(year, month, day, hour, minute, second);
    if (out->time == TIMESTAMP_INVALID) {
        return 0;
    }

    if (!copy_field(field, sizeof field, bar1 + 1, bar2)) {
        return 0;
    }
    out->level = level_from_word(field);
    if (out->level == LEVEL_UNKNOWN) {
        return 0;
    }

    if (bar3 == bar2 + 1) {
        return 0;
    }
    if (!copy_field(out->module, sizeof out->module, bar2 + 1, bar3)) {
        return 0;
    }

    size_t text_length = strlen(bar3 + 1);
    if (text_length >= sizeof out->text) {
        text_length = sizeof out->text - 1;
    }
    memcpy(out->text, bar3 + 1, text_length);
    out->text[text_length] = '\0';
    return 1;
}
