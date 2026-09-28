#ifndef COMMAND_H
#define COMMAND_H

typedef enum {
    CMD_UNKNOWN = 0,
    CMD_NEW,
    CMD_LOOK,
    CMD_NORTH,
    CMD_SOUTH,
    CMD_EAST,
    CMD_WEST,
    CMD_STATUS,
    CMD_HIT,
    CMD_REST,
    CMD_POISON,
    CMD_HELP,
    CMD_QUIT,
    CMD_EMPTY,
    CMD_EOF
} Command;

Command command_read();

const char *command_last_word();

#endif // COMMAND_H
