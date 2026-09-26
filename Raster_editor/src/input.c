#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "input.h"

int input_read_line(char *buffer, size_t size)
{
    if (buffer == NULL || size == 0) {
        return 0;
    }
    if (fgets(buffer, (int)size, stdin) == NULL) {
        buffer[0] = '\0';
        return 0;
    }
    if (strchr(buffer, '\n') == NULL) {
        int c;
        while ((c = getchar()) != EOF && c != '\n') {}
    }

    size_t length = strlen(buffer);
    while (length > 0 && isspace((unsigned char)buffer[length - 1])) {
        buffer[--length] = '\0';
    }

    size_t start = 0;
    while (buffer[start] != '\0' && isspace((unsigned char)buffer[start])) {
        ++start;
    }
    if (start > 0) {
        memmove(buffer, buffer + start, strlen(buffer + start) + 1);
    }
    for (size_t i = 0; buffer[i] != '\0'; ++i) {
        buffer[i] = (char)tolower((unsigned char)buffer[i]);
    }
    return 1;
}
