#include <stdio.h>
#include <stdlib.h>

#include "generator.h"
#include "level.h"

static const char *const MODULES[] = { "net", "db", "ui", "auth", "cache", "disk" };
static const char *const MESSAGES[] = {
    "process request",
    "connection established",
    "connection terminated",
    "timeout exceeded",
    "user|admin logged in",
    "configuration file not found",
    "retry after 5 seconds",
    "cache cleared"
};

static Level random_level() {
    const int r = rand() % 100;

    if (r < 30) return LEVEL_DEBUG;
    if (r < 70) return LEVEL_INFO;
    if (r < 85) return LEVEL_WARN;
    if (r < 97) return LEVEL_ERROR;
    return LEVEL_FATAL;
}

int generator_write(const char *path, const long count, const unsigned int seed) {
    FILE *file = fopen(path, "w");
    const int module_count = (int)(sizeof MODULES / sizeof MODULES[0]);
    const int message_count = (int)(sizeof MESSAGES / sizeof MESSAGES[0]);

    if (file == NULL) {
        return 0;
    }
    srand(seed);

    for (long i = 0; i < count; ++i) {
        fprintf(file, "2026-03-%02d %02d:%02d:%02d|%s|%s|%s #%ld\n",
                1 + rand() % 28, rand() % 24, rand() % 60, rand() % 60,
                level_name(random_level()),
                MODULES[rand() % module_count],
                MESSAGES[rand() % message_count],
                i + 1);
    }
    fclose(file);
    return 1;
}
