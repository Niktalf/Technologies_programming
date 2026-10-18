#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "level.h"

static const char *const LEVEL_NAMES[LEVEL_COUNT] = {
    "DEBUG", "INFO", "WARN", "ERROR", "FATAL"
};

/* Сравнивает слово с именем уровня без учёта регистра.
   Имена уровней записаны заглавными, поэтому заглавной делается только буква слова. */
static int equals_ignore_case(const char *word, const char *name) {
    size_t i = 0;
    while (name[i] != '\0' && toupper((unsigned char)word[i]) == name[i]) {
        ++i;
    }
    /* Совпадение — только если оба слова кончились одновременно. */
    return name[i] == '\0' && word[i] == '\0';
}

Level level_from_word(const char *word) {
    if (word == NULL || word[0] == '\0') {
        return LEVEL_UNKNOWN;
    }
    for (int i = 0; i < LEVEL_COUNT; ++i) {
        if (equals_ignore_case(word, LEVEL_NAMES[i])) {
            return (Level)i;
        }
    }
    return LEVEL_UNKNOWN;
}

const char *level_name(Level level) {
    if (level < 0 || level >= LEVEL_COUNT) {
        return "UNKNOWN";
    }
    return LEVEL_NAMES[level];
}

LevelMask level_mask_set(LevelMask mask, Level level) {
    if (level < 0 || level >= LEVEL_COUNT) {
        return mask;
    }
    return (LevelMask)(mask | LEVEL_BIT(level));
}

LevelMask level_mask_clear(LevelMask mask, Level level) {
    if (level < 0 || level >= LEVEL_COUNT) {
        return mask;
    }
    /* Снятие бита: И с инверсией. Ошибочное ИЛИ здесь ничего не снимет. */
    return (LevelMask)(mask & (LevelMask)~LEVEL_BIT(level));
}

int level_mask_has(LevelMask mask, Level level) {
    if (level < 0 || level >= LEVEL_COUNT) {
        return 0;
    }
    /* Результат И — значение бита, а не единица. Сравниваем с нулём. */
    return (mask & LEVEL_BIT(level)) != 0;
}

void level_mask_print(LevelMask mask) {
    printf("Level filter: ");
    int printed = 0;
    for (int i = 0; i < LEVEL_COUNT; ++i) {
        if (level_mask_has(mask, (Level)i)) {
            printf("%s%s", printed ? ", " : "", LEVEL_NAMES[i]);
            printed = 1;
        }
    }
    if (!printed) {
        printf("empty (no level passes)");
    }
    printf("\nMask: 0x%02X\n", (unsigned)mask);
}
