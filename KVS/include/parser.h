#ifndef PARSER_H
#define PARSER_H

#define PARSER_ARGS_MAX 8

typedef enum {
    PARSER_OK = 0,
    PARSER_ERR_QUOTE,
    PARSER_ERR_TOO_MANY
} ParserStatus;

const char *parser_status_text(ParserStatus status);
ParserStatus parser_split(char *line, char **argv, int max_args, int *out_count);

#endif // PARSER_H
