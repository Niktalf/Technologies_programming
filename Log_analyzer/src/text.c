#include <ctype.h>
#include <stddef.h>

#include "text.h"

static long length_of(const char *text)
{
    long length = 0;

    while (text[length] != '\0') {
        ++length;
    }
    return length;
}

static long find(const char *haystack, const char *needle, const int ignore_case) {
    if (haystack == NULL || needle == NULL) {
        return -1;
    }

    const long needle_length = length_of(needle);
    if (needle_length == 0) {
        return 0;
    }

    const long haystack_length = length_of(haystack);
    if (needle_length > haystack_length) {
        return -1;
    }

    for (long start = 0; start + needle_length <= haystack_length; ++start) {
        long i = 0;
        while (i < needle_length) {
            char a = haystack[start + i];
            char b = needle[i];

            if (ignore_case) {
                a = (char)tolower((unsigned char)a);
                b = (char)tolower((unsigned char)b);
            }
            if (a != b) {
                break;
            }
            ++i;
        }
        if (i == needle_length) {
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
    while (a[i] != '\0' && b[i] != '\0') {
        if (tolower((unsigned char)a[i]) != tolower((unsigned char)b[i])) {
            return 0;
        }
        ++i;
    }
    return a[i] == '\0' && b[i] == '\0';
}
