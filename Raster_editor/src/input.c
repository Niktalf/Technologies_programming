#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "input.h"

static void lower_first_word(const Words *words) {
    if (words->count == 0) {
        return;
    }
    for (int i = 0; words->word[0][i] != '\0'; ++i) {
        words->word[0][i] = (char)tolower((unsigned char)words->word[0][i]);
    }
}

int input_read_words(Words *words) {
    words->count = 0;

    if (fgets(words->line, INPUT_LINE_MAX, stdin) == NULL) {
        return 0;
    }
    if (strchr(words->line, '\n') == NULL) {
        int c;
        while ((c = getchar()) != EOF && c != '\n') {}
    }

    char *token = strtok(words->line, " \t\r\n");
    while (token != NULL && words->count < INPUT_WORDS_MAX) {
        words->word[words->count++] = token;
        token = strtok(NULL, " \t\r\n");
    }
    lower_first_word(words);
    return 1;
}

void input_from_args(Words *words, const int argc, char **argv) {
    words->count = 0;
    words->line[0] = '\0';
    for (int i = 0; i < argc && i < INPUT_WORDS_MAX; ++i) {
        words->word[words->count++] = argv[i];
    }
}
