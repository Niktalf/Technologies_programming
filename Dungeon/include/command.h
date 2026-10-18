#ifndef COMMAND_H
#define COMMAND_H

/* Практика 6: разбор введённой строки вручную.

   Строка читается целиком, затем разбивается на слова прямо в буфере:
   пробелы заменяются нулевыми символами, а в массив указателей
   записываются адреса начал слов. Ничего не копируется, и каждое слово —
   готовая строка C с нулём на конце. */

#define LINE_SIZE  128   /* 126 символов, перевод строки и завершающий ноль */
#define WORDS_MAX  8

/* Результат чтения строки. */
#define READ_OK       1
#define READ_END      0    /* ввод кончился */
#define READ_TOO_LONG (-1) /* строка не поместилась; её хвост выброшен */

int command_read_line(char *line, int size);

/* Разбивает строку на слова. Возвращает их число; лишние слова сверх
   max_words не попадают в массив. */
int command_split(char *line, char *words[], int max_words);

/* Переводит слово в нижний регистр на месте. Только латиница:
   tolower из стандартной библиотеки не знает про русские буквы в UTF-8. */
void command_to_lower(char *word);

/* Совпадает ли слово с полным названием или с сокращением.
   Сокращение может быть NULL. */
int command_is(const char *word, const char *full, const char *abbreviation);

/* Результат перевода слова в число. */
#define NUMBER_OK        0
#define NUMBER_MISSING   1   /* аргумента нет */
#define NUMBER_NOT_DIGIT 2   /* в слове есть не цифра */
#define NUMBER_TOO_BIG   3   /* не помещается в int */

/* Переводит слово из цифр в неотрицательное число, проходя по символам.
   Возвращает один из кодов NUMBER_*; число — через *out. */
int command_parse_number(const char *word, int *out);

/* Сообщение для кода NUMBER_*. */
const char *command_number_error(int code);

#endif /* COMMAND_H */
