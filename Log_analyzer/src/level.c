#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "level.h"

static const char *const LEVEL_NAMES[LEVEL_COUNT] = {
    "DEBUG", "INFO", "WARN", "ERROR", "FATAL"
};

Level level_from_word(const char *word)
{
    if (word == NULL || word[0] == '\0') {
        return LEVEL_UNKNOWN;
    }
    for (int i = 0; i < LEVEL_COUNT; ++i) {
        size_t j = 0;
        const char *name = LEVEL_NAMES[i];
        int equal = 1;

        while (name[j] != '\0' && word[j] != '\0') {
            if (toupper((unsigned char)word[j]) != name[j]) {
                equal = 0;
                break;
            }
            ++j;
        }
        if (equal && name[j] == '\0' && word[j] == '\0') {
            return (Level)i;
        }
    }
    return LEVEL_UNKNOWN;
}

const char *level_name(const Level level)
{
    if (level < 0 || level >= LEVEL_COUNT) {
        return "UNKNOWN";
    }
    return LEVEL_NAMES[level];
}

LevelMask level_mask_set(const LevelMask mask, const Level level)
{
    if (level < 0 || level >= LEVEL_COUNT) {
        return mask;
    }
    return (LevelMask)(mask | LEVEL_BIT(level));
}

LevelMask level_mask_clear(const LevelMask mask, const Level level)
{
    if (level < 0 || level >= LEVEL_COUNT) {
        return mask;
    }
    return (LevelMask)(mask & (LevelMask)~LEVEL_BIT(level));
}

int level_mask_has(const LevelMask mask, const Level level)
{
    if (level < 0 || level >= LEVEL_COUNT) {
        return 0;
    }
    return (mask & LEVEL_BIT(level)) != 0;
}

void level_mask_print(const LevelMask mask)
{
    int printed = 0;

    printf("Level filter: ");
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
