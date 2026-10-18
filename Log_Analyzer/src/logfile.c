#include <stdio.h>
#include <string.h>

#include "logfile.h"

void logfile_init(LogFile *log) {
    if (log == NULL) {
        return;
    }
    memset(log, 0, sizeof *log);
}

const char *logfile_status_text(LogStatus status) {
    switch (status) {
    case LOG_OK:
        return "success";
    case LOG_ERR_ARG:
        return "invalid argument";
    case LOG_ERR_OPEN:
        return "cannot open file";
    default:
        return "unknown error";
    }
}

/* Убирает завершающий перевод строки, если он есть. */
static void strip_newline(char *line) {
    size_t length = strlen(line);
    if (length > 0 && line[length - 1] == '\n') {
        line[length - 1] = '\0';
    }
}

/* Дочитывает и выбрасывает остаток строки до перевода строки или конца файла. */
static void skip_rest_of_line(FILE *file) {
    int c = fgetc(file);
    while (c != '\n' && c != EOF) {
        c = fgetc(file);
    }
}

Level logfile_line_level(const char *line) {
    if (line == NULL) {
        return LEVEL_UNKNOWN;
    }
    /* Уровень — второе поле: между первой и второй вертикальной чертой. */
    const char *start = strchr(line, '|');
    if (start == NULL) {
        return LEVEL_UNKNOWN;
    }
    ++start;

    char word[16];
    size_t length = 0;
    while (start[length] != '\0' && start[length] != '|') {
        if (length + 1 >= sizeof word) {
            return LEVEL_UNKNOWN; /* слишком длинное для уровня */
        }
        word[length] = start[length];
        ++length;
    }
    if (start[length] != '|') {
        return LEVEL_UNKNOWN; /* второй черты нет: строка не по формату */
    }
    word[length] = '\0';
    return level_from_word(word);
}

/* Проходит ли строка фильтр. Без фильтра проходят все строки, даже
   испорченные. С фильтром строка с неизвестным уровнем не проходит:
   иначе опечатка в уровне попала бы в выборку «только ошибки». */
static int passes(const char *line, LevelMask mask) {
    if (mask == LEVEL_MASK_ALL) {
        return 1;
    }
    Level level = logfile_line_level(line);
    return level != LEVEL_UNKNOWN && level_mask_has(mask, level);
}

/* Запоминает строку, прошедшую фильтр: в первые, если их ещё меньше десяти,
   и в кольцевой буфер последних. Рядом сохраняется номер строки в файле. */
static void remember_line(LogFile *log, const char *line) {
    if (log->matched < LOGFILE_HEAD_SIZE) {
        strcpy(log->head[log->matched], line);
        log->head_number[log->matched] = log->total;
    }
    /* Кольцевой буфер: новая строка занимает место самой старой. */
    long slot = log->matched % LOGFILE_TAIL_SIZE;
    strcpy(log->tail[slot], line);
    log->tail_number[slot] = log->total;
    ++log->matched;
}

LogStatus logfile_scan(LogFile *log, const char *path, LevelMask mask) {
    if (log == NULL || path == NULL) {
        return LOG_ERR_ARG;
    }
    size_t path_length = strlen(path);
    if (path_length >= LOGFILE_PATH_MAX) {
        return LOG_ERR_ARG;
    }
    /* Путь сохраняется до очистки структуры: при повторном просмотре
       сюда передают log->path, и очистка стёрла бы его раньше, чем он
       понадобится. */
    char saved_path[LOGFILE_PATH_MAX];
    memcpy(saved_path, path, path_length + 1);

    FILE *file = fopen(path, "r");
    if (file == NULL) {
        return LOG_ERR_OPEN;
    }

    logfile_init(log);
    memcpy(log->path, saved_path, path_length + 1);
    log->mask = mask;

    char line[LOGFILE_LINE_MAX];
    while (fgets(line, (int)sizeof line, file) != NULL) {
        /* Строка длиннее буфера: дочитываем хвост, чтобы он не стал новой строкой. */
        if (strchr(line, '\n') == NULL && !feof(file)) {
            skip_rest_of_line(file);
            ++log->truncated;
        }
        strip_newline(line);
        ++log->total;

        if (logfile_line_level(line) == LEVEL_UNKNOWN) {
            ++log->unknown;
        }
        if (passes(line, mask)) {
            remember_line(log, line);
        }
    }

    fclose(file);
    log->loaded = 1;
    return LOG_OK;
}

/* Сколько строк печатать: не больше запрошенного, размера буфера и числа строк. */
static long print_limit(const LogFile *log, int count, int buffer_size) {
    long limit = count < buffer_size ? count : buffer_size;
    if (limit > log->matched) {
        limit = log->matched;
    }
    return limit;
}

void logfile_print_head(const LogFile *log, int count) {
    if (log == NULL || !log->loaded) {
        printf("Log is not loaded.\n");
        return;
    }
    long limit = print_limit(log, count, LOGFILE_HEAD_SIZE);
    for (long i = 0; i < limit; ++i) {
        printf("%ld: %s\n", log->head_number[i], log->head[i]);
    }
}

void logfile_print_tail(const LogFile *log, int count) {
    if (log == NULL || !log->loaded) {
        printf("Log is not loaded.\n");
        return;
    }
    long limit = print_limit(log, count, LOGFILE_TAIL_SIZE);
    for (long i = log->matched - limit; i < log->matched; ++i) {
        /* Тот же остаток от деления, что при записи, даёт верный порядок. */
        long slot = i % LOGFILE_TAIL_SIZE;
        printf("%ld: %s\n", log->tail_number[slot], log->tail[slot]);
    }
}
