#ifndef COMMAND_H
#define COMMAND_H

#define COMMAND_LINE_MAX  128
#define COMMAND_WORDS_MAX 8
#define COMMAND_WORD_MAX  32

typedef struct {
    char  line[COMMAND_LINE_MAX];
    char *word[COMMAND_WORDS_MAX];
    int   count;
    int   eof;
} Command;

void command_read(Command *command);
const char *command_name(const Command *command);
const char *command_argument(const Command *command, int index);

#endif // COMMAND_H
