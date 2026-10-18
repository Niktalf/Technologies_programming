#ifndef INPUT_H
#define INPUT_H

/* Чтение команды с аргументами.
   Строка режется на слова по пробелам. К нижнему регистру приводится
   только первое слово — имя команды: остальные могут быть именами файлов,
   и менять в них регистр нельзя. */

#define INPUT_LINE_MAX  256
#define INPUT_WORDS_MAX 8

typedef struct {
    char line[INPUT_LINE_MAX];  /* строка, разрезанная на слова */
    char *word[INPUT_WORDS_MAX]; /* указатели на слова внутри line */
    int count;                   /* сколько слов найдено */
} Words;

/* Возвращает 1, если строка прочитана, и 0, если ввод закончился. */
int input_read_words(Words *words);

/* Собирает слова из аргументов командной строки, начиная с первого. */
void input_from_args(Words *words, int argc, char *argv[]);

#endif /* INPUT_H */
