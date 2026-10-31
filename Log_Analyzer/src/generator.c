#include <stdio.h>
#include <stdlib.h>

#include "generator.h"
#include "level.h"

static const char *const MODULES[] = { "net", "db", "ui", "auth", "cache", "disk" };
static const char *const MESSAGES[] = {
    "request processed",
    "connection established",
    "connection lost",
    "timeout exceeded",
    "user|admin logged in",
    "configuration file not found",
    "retrying in 5 seconds",
    "cache cleared"
};

/* Уровни с весами: отладочных и информационных сообщений больше всего,
   критических — единицы. Сумма весов — 100. */
static Level random_level(void) {
    int r = rand() % 100;
    if (r < 30) {
        return LEVEL_DEBUG;
    }
    if (r < 70) {
        return LEVEL_INFO;
    }
    if (r < 85) {
        return LEVEL_WARN;
    }
    if (r < 97) {
        return LEVEL_ERROR;
    }
    return LEVEL_FATAL;
}

LogStatus generator_write(const char *path, long count, unsigned int seed) {
    if (path == NULL || count < 0) {
        return LOG_ERR_ARG;
    }
    FILE *file = fopen(path, "w");
    if (file == NULL) {
        return LOG_ERR_OPEN;
    }
    srand(seed);

    const int module_count = (int)(sizeof MODULES / sizeof MODULES[0]);
    const int message_count = (int)(sizeof MESSAGES / sizeof MESSAGES[0]);
    int written = 1;
    for (long i = 0; i < count && written; ++i) {
        /* Порядок вызовов rand внутри одного вызова printf не определён,
           поэтому каждое случайное значение берётся отдельной инструкцией:
           только так файл с тем же зерном совпадает на любом компиляторе. */
        int day = 1 + rand() % 28;
        int hour = rand() % 24;
        int minute = rand() % 60;
        int second = rand() % 60;
        Level level = random_level();
        const char *module = MODULES[rand() % module_count];
        const char *message = MESSAGES[rand() % message_count];
        /* fprintf возвращает отрицательное число при ошибке записи. */
        written = fprintf(file, "2026-03-%02d %02d:%02d:%02d|%s|%s|%s #%ld\n",
                          day, hour, minute, second, level_name(level), module, message,
                          i + 1) >= 0;
    }
    /* Часть ошибок записи видна только при закрытии: данные из буфера
       уходят на диск именно тогда. */
    if (fclose(file) != 0 || !written) {
        return LOG_ERR_WRITE;
    }
    return LOG_OK;
}
