#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "input.h"

/* Дочитывает и выбрасывает остаток строки до перевода строки или конца ввода. */
static void skip_rest_of_line(void) {
    int c = getchar();
    while (c != '\n' && c != EOF) {
        c = getchar();
    }
}

int input_read_line(char *buffer, size_t size) {
    if (buffer == NULL || size == 0) {
        return 0;
    }
    /* fgets читает не больше size - 1 символов и всегда ставит ноль в конце. */
    if (fgets(buffer, (int)size, stdin) == NULL) {
        buffer[0] = '\0';
        return 0;
    }
    if (strchr(buffer, '\n') == NULL) {
        skip_rest_of_line();
    }

    size_t length = strlen(buffer);
    while (length > 0 && isspace((unsigned char)buffer[length - 1])) {
        --length;
        buffer[length] = '\0';
    }

    size_t start = 0;
    while (buffer[start] != '\0' && isspace((unsigned char)buffer[start])) {
        ++start;
    }
    if (start > 0) {
        memmove(buffer, buffer + start, length - start + 1);
    }

    for (size_t i = 0; buffer[i] != '\0'; ++i) {
        buffer[i] = (char)tolower((unsigned char)buffer[i]);
    }
    return 1;
}
