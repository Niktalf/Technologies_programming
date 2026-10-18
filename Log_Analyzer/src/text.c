#include <ctype.h>
#include <stddef.h>

#include "text.h"

/* Длина строки: идём по ней, пока не встретим нулевой символ.
   Длина нигде не хранится, поэтому вызывать такую функцию в условии
   цикла по строке — значит превращать проход в квадратичный. */
static long length_of(const char *text) {
    long length = 0;
    while (text[length] != '\0') {
        ++length;
    }
    return length;
}

/* Совпадают ли два символа: точно или без учёта регистра. */
static int same_char(char a, char b, int ignore_case) {
    if (ignore_case) {
        return tolower((unsigned char)a) == tolower((unsigned char)b);
    }
    return a == b;
}

/* Совпадает ли образец длины length с текстом, начиная с начала text. */
static int matches_at(const char *text, const char *needle, long length, int ignore_case) {
    long i = 0;
    while (i < length && same_char(text[i], needle[i], ignore_case)) {
        ++i;
    }
    return i == length;
}

static long find(const char *haystack, const char *needle, int ignore_case) {
    if (haystack == NULL || needle == NULL) {
        return -1;
    }
    long haystack_length = length_of(haystack);
    long needle_length = length_of(needle);
    if (needle_length == 0) {
        return 0;
    }
    /* Условие цикла — нестрогое: последнее вхождение начинается ровно
       в позиции haystack_length - needle_length. При строгом неравенстве
       оно потерялось бы. Если образец длиннее текста, цикл не выполнится
       ни разу. */
    for (long start = 0; start + needle_length <= haystack_length; ++start) {
        if (matches_at(haystack + start, needle, needle_length, ignore_case)) {
            return start;
        }
    }
    return -1;
}

long text_find(const char *haystack, const char *needle) {
    return find(haystack, needle, 0);
}

long text_find_ignore_case(const char *haystack, const char *needle) {
    return find(haystack, needle, 1);
}

int text_same_word(const char *a, const char *b) {
    if (a == NULL || b == NULL) {
        return 0;
    }
    long i = 0;
    while (a[i] != '\0' && same_char(a[i], b[i], 1)) {
        ++i;
    }
    /* Совпадение — только если оба слова кончились одновременно. */
    return a[i] == '\0' && b[i] == '\0';
}
