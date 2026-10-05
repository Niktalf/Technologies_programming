#include <ctype.h>
#include <stddef.h>

#include "parser.h"

const char *parser_status_text(const ParserStatus status){
    switch (status) {
        case PARSER_OK:           return "success";
        case PARSER_ERR_QUOTE:    return "the quotation mark is not closed";
        case PARSER_ERR_TOO_MANY: return "too many words";
        default:                  return "unknown error";
    }
}

ParserStatus parser_split(char *line, char **argv, const int max_args, int *out_count) {
    if (line == NULL || argv == NULL || out_count == NULL) {
        return PARSER_ERR_TOO_MANY;
    }
    *out_count = 0;

    const char *read = line;
    char *write = line;
    int count = 0;
    while (*read != '\0') {
        while (*read != '\0' && isspace((unsigned char)*read)) {
            ++read;
        }
        if (*read == '\0') {
            break;
        }
        if (count >= max_args) {
            return PARSER_ERR_TOO_MANY;
        }

        argv[count] = write;
        ++count;

        int in_quotes = 0;
        while (*read != '\0') {
            if (*read == '\\' && read[1] != '\0') {
                ++read;
                *write++ = *read++;
                continue;
            }
            if (*read == '"') {
                in_quotes = !in_quotes;
                ++read;
                continue;
            }
            if (!in_quotes && isspace((unsigned char)*read)) {
                break;
            }
            *write++ = *read++;
        }

        if (in_quotes) {
            return PARSER_ERR_QUOTE;
        }

        if (*read != '\0') {
            ++read;
        }
        *write++ = '\0';
    }

    *out_count = count;
    return PARSER_OK;
}
