#ifndef INPUT_H
#define INPUT_H

#define INPUT_LINE_MAX  256
#define INPUT_WORDS_MAX 8

typedef struct {
    char  line[INPUT_LINE_MAX];
    char *word[INPUT_WORDS_MAX];
    int   count;
} Words;

int input_read_words(Words *words);
void input_from_args(Words *words, int argc, char **argv);

#endif // INPUT_H
