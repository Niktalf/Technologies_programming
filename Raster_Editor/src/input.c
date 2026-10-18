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

static void lower_first_word(Words *words) {
    if (words->count == 0) {
        return;
    }
    char *name = words->word[0];
    for (size_t i = 0; name[i] != '\0'; ++i) {
        name[i] = (char)tolower((unsigned char)name[i]);
    }
}

int input_read_words(Words *words) {
    words->count = 0;
    if (fgets(words->line, INPUT_LINE_MAX, stdin) == NULL) {
        return 0;
    }
    /* Строка длиннее буфера: хвост дочитываем, иначе он станет следующей командой. */
    if (strchr(words->line, '\n') == NULL) {
        skip_rest_of_line();
    }

    /* strtok заменяет разделители нулями и возвращает начала слов внутри line.
       Слова сверх INPUT_WORDS_MAX отбрасываются. */
    char *token = strtok(words->line, " \t\r\n");
    while (token != NULL && words->count < INPUT_WORDS_MAX) {
        words->word[words->count] = token;
        ++words->count;
        token = strtok(NULL, " \t\r\n");
    }
    lower_first_word(words);
    return 1;
}

void input_from_args(Words *words, int argc, char *argv[]) {
    /* Аргументы программы менять нельзя, поэтому имя команды здесь
       к нижнему регистру не приводится: регистр не учитывается в самом
       разборе команд. */
    words->count = 0;
    words->line[0] = '\0';
    for (int i = 0; i < argc && i < INPUT_WORDS_MAX; ++i) {
        words->word[words->count] = argv[i];
        ++words->count;
    }
}
