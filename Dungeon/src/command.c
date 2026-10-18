#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "command.h"

/* Дочитывает и выбрасывает остаток строки до перевода строки или конца ввода. */
static void skip_rest_of_line(void) {
    int c = getchar();
    while (c != '\n' && c != EOF) {
        c = getchar();
    }
}

int command_read_line(char *line, int size) {
    if (line == NULL || size < 2) {
        return READ_END;
    }
    /* fgets читает не больше size - 1 символов и всегда ставит ноль
       в конце, поэтому переполнить буфер нельзя при любом вводе. */
    if (fgets(line, size, stdin) == NULL) {
        return READ_END;
    }
    char *newline = strchr(line, '\n');
    if (newline != NULL) {
        *newline = '\0';
        return READ_OK;
    }
    if (feof(stdin)) {
        return READ_OK; /* последняя строка без перевода строки — тоже строка */
    }
    /* Перевода строки нет — строка длиннее буфера. Хвост дочитываем
       и выбрасываем, иначе он стал бы следующей командой. */
    skip_rest_of_line();
    return READ_TOO_LONG;
}

static int is_space(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

int command_split(char *line, char *words[], int max_words) {
    if (line == NULL || words == NULL) {
        return 0;
    }
    /* Один проход указателем по строке. Длина не вычисляется вовсе:
       конец строки — это нулевой символ, на нём и останавливаемся. */
    int count = 0;
    char *p = line;
    while (*p != '\0') {
        while (is_space(*p)) {
            *p = '\0'; /* разделитель становится концом предыдущего слова */
            ++p;
        }
        if (*p != '\0') {
            if (count < max_words) {
                words[count] = p;
            }
            ++count;
            while (*p != '\0' && !is_space(*p)) {
                ++p;
            }
        }
    }
    return count < max_words ? count : max_words;
}

void command_to_lower(char *word) {
    if (word == NULL) {
        return;
    }
    for (char *p = word; *p != '\0'; ++p) {
        if (*p >= 'A' && *p <= 'Z') {
            *p = (char)(*p - 'A' + 'a');
        }
    }
}

int command_is(const char *word, const char *full, const char *abbreviation) {
    if (word == NULL) {
        return 0;
    }
    /* Строки сравниваются по содержимому. word == full сравнило бы адреса. */
    if (strcmp(word, full) == 0) {
        return 1;
    }
    return abbreviation != NULL && strcmp(word, abbreviation) == 0;
}

int command_parse_number(const char *word, int *out) {
    if (out == NULL || word == NULL || *word == '\0') {
        return NUMBER_MISSING;
    }
    int value = 0;
    for (const char *p = word; *p != '\0'; ++p) {
        if (*p < '0' || *p > '9') {
            return NUMBER_NOT_DIGIT;
        }
        int digit = *p - '0'; /* код цифры минус код нуля — значение цифры */
        /* Проверка до умножения: после переполнения проверять уже нечего,
           а само переполнение знакового int — неопределённое поведение. */
        if (value > (INT_MAX - digit) / 10) {
            return NUMBER_TOO_BIG;
        }
        value = value * 10 + digit;
    }
    *out = value;
    return NUMBER_OK;
}

const char *command_number_error(int code) {
    switch (code) {
    case NUMBER_MISSING:
        return "A number is missing";
    case NUMBER_NOT_DIGIT:
        return "Not a number: digits only";
    case NUMBER_TOO_BIG:
        return "The number is too large";
    default:
        return "";
    }
}
