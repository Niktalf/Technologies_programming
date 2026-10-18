#include <ctype.h>
#include <stddef.h>

#include "parser.h"

const char *parser_status_text(ParserStatus status) {
    switch (status) {
    case PARSER_OK:
        return "success";
    case PARSER_ERR_QUOTE:
        return "unclosed quote";
    case PARSER_ERR_TOO_MANY:
        return "too many words";
    default:
        return "unknown error";
    }
}

/* Читает одно слово, начиная с *read, и пишет очищенное слово начиная
   с *write. Оба указателя передаются по адресу и сдвигаются: после вызова
   *read стоит на разделителе или на конце строки.

   Запись идёт в ту же строку, но левее или на то же место: очищенное слово
   всегда короче исходного куска, потому что кавычки и косые черты исчезают.
   Возвращает 0, если кавычка осталась незакрытой. */
static int read_word(char **read, char **write) {
    char *r = *read;
    char *w = *write;
    int in_quotes = 0;
    while (*r != '\0' && (in_quotes || !isspace((unsigned char)*r))) {
        if (*r == '\\' && r[1] != '\0') {
            /* Обратная косая черта означает: следующий символ — обычный. */
            ++r;
            *w = *r;
            ++w;
            ++r;
        } else if (*r == '"') {
            in_quotes = !in_quotes;
            ++r;
        } else {
            *w = *r;
            ++w;
            ++r;
        }
    }
    *read = r;
    *write = w;
    return !in_quotes;
}

ParserStatus parser_split(char *line, char *argv[], int max_args, int *out_count) {
    if (line == NULL || argv == NULL || out_count == NULL) {
        return PARSER_ERR_TOO_MANY;
    }
    *out_count = 0;

    char *read = line;  /* откуда читаем */
    char *write = line; /* куда складываем очищенное слово */
    int count = 0;
    while (*read != '\0') {
        /* Пропускаем пробелы между словами. */
        while (*read != '\0' && isspace((unsigned char)*read)) {
            ++read;
        }
        if (*read != '\0') {
            if (count >= max_args) {
                return PARSER_ERR_TOO_MANY;
            }
            argv[count] = write;
            ++count;
            if (!read_word(&read, &write)) {
                return PARSER_ERR_QUOTE;
            }
            /* Сначала шагаем через разделитель, и только потом ставим нулевой
               символ. Обратный порядок затёр бы тот самый пробел, на который
               ещё смотрит указатель чтения, и разбор закончился бы на первом
               слове. */
            if (*read != '\0') {
                ++read;
            }
            *write = '\0';
            ++write;
        }
    }

    *out_count = count;
    return PARSER_OK;
}
