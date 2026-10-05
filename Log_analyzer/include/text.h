#ifndef TEXT_H
#define TEXT_H

long text_find(const char *haystack, const char *needle);
long text_find_ignore_case(const char *haystack, const char *needle);
int text_same_word(const char *a, const char *b);

#endif // TEXT_H