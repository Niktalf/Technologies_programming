#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "command.h"

#define COMMAND_BUFFER_SIZE 64

static char last_word[COMMAND_BUFFER_SIZE];

static void discard_rest_of_line(const char *buffer)
{
    int c;

    if (strchr(buffer, '\n') != NULL) {
        return;
    }
    while ((c = getchar()) != EOF && c != '\n') {}
}

static void trim(char *buffer)
{
    size_t length = strlen(buffer);
    size_t start = 0;

    while (length > 0 && isspace((unsigned char)buffer[length - 1])) {
        buffer[--length] = '\0';
    }
    while (buffer[start] != '\0' && isspace((unsigned char)buffer[start])) {
        ++start;
    }
    if (start > 0) {
        memmove(buffer, buffer + start, strlen(buffer + start) + 1);
    }
}

static void to_lower(char *buffer)
{
    size_t i;

    for (i = 0; buffer[i] != '\0'; ++i) {
        buffer[i] = (char)tolower((unsigned char)buffer[i]);
    }
}

Command command_read()
{
    char buffer[COMMAND_BUFFER_SIZE];

    if (fgets(buffer, (int)sizeof buffer, stdin) == NULL) {
        last_word[0] = '\0';
        return CMD_EOF;
    }
    discard_rest_of_line(buffer);
    trim(buffer);
    to_lower(buffer);

    strncpy(last_word, buffer, sizeof last_word - 1);
    last_word[sizeof last_word - 1] = '\0';

    if (buffer[0] == '\0') {
        return CMD_EMPTY;
    }
    if (strcmp(buffer, "new") == 0) {
        return CMD_NEW;
    }
    if (strcmp(buffer, "look") == 0) {
        return CMD_LOOK;
    }
    if (strcmp(buffer, "north") == 0 || strcmp(buffer, "n") == 0) {
        return CMD_NORTH;
    }
    if (strcmp(buffer, "south") == 0 || strcmp(buffer, "s") == 0) {
        return CMD_SOUTH;
    }
    if (strcmp(buffer, "east") == 0 || strcmp(buffer, "e") == 0) {
        return CMD_EAST;
    }
    if (strcmp(buffer, "west") == 0 || strcmp(buffer, "w") == 0) {
        return CMD_WEST;
    }
    if (strcmp(buffer, "reach") == 0) {
        return CMD_REACH;
    }
    if (strcmp(buffer, "status") == 0) {
        return CMD_STATUS;
    }
    if (strcmp(buffer, "hit") == 0) {
        return CMD_HIT;
    }
    if (strcmp(buffer, "rest") == 0) {
        return CMD_REST;
    }
    if (strcmp(buffer, "poison") == 0) {
        return CMD_POISON;
    }
    if (strcmp(buffer, "help") == 0) {
        return CMD_HELP;
    }
    if (strcmp(buffer, "quit") == 0) {
        return CMD_QUIT;
    }
    return CMD_UNKNOWN;
}

const char *command_last_word()
{
    return last_word;
}
