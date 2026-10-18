#ifndef TEXT_H
#define TEXT_H

/* Практика 6: поиск подстроки, написанный вручную.
   Библиотечная функция поиска здесь не используется намеренно:
   смысл задания в том, чтобы разобраться, как он устроен. */

/* Возвращает позицию первого вхождения needle в haystack или -1.
   Пустой образец считается найденным в позиции 0 — так же,
   как это делает библиотечная функция. */
long text_find(const char *haystack, const char *needle);

/* То же, но большие и малые буквы считаются одинаковыми. */
long text_find_ignore_case(const char *haystack, const char *needle);

/* Сравнение слов без учёта регистра. */
int text_same_word(const char *a, const char *b);

#endif /* TEXT_H */
