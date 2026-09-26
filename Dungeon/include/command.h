#ifndef COMMAND_H
#define COMMAND_H


typedef enum {
    CMD_UNKNOWN = 0,
    CMD_NEW,
    CMD_LOOK,
    CMD_HELP,
    CMD_QUIT,
    CMD_EMPTY,
    CMD_EOF
} Command;

Command command_read(void);

const char *command_last_word(void);

#endif // COMMAND_H
