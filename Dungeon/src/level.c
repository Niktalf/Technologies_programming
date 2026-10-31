#include <stdio.h>
#include <string.h>

#include "level.h"

/* Строка файла: ширина карты, \r, \n и завершающий ноль. */
#define LEVEL_LINE_SIZE (MAP_MAX_WIDTH + 3)

/* Сырые строки уровня, прочитанные из файла. Карта ещё не создана:
   её размер станет известен, только когда файл прочитан целиком. */
typedef struct {
    char rows[MAP_MAX_HEIGHT][MAP_MAX_WIDTH];
    int width;
    int height;
} RawLevel;

/* Убирает из конца строки \n и \r. Возвращает новую длину. */
static int strip_line_end(char *line) {
    int length = (int)strlen(line);
    while (length > 0 && (line[length - 1] == '\n' || line[length - 1] == '\r')) {
        line[length - 1] = '\0';
        --length;
    }
    return length;
}

/* Читает файл в raw и проверяет форму: размеры, одинаковую длину строк.
   Файл закрывается здесь же, на всех путях выхода. */
static Status read_raw(const char *path, RawLevel *raw, int *out_line) {
    FILE *file = fopen(path, "r");
    if (file == NULL) {
        return STATUS_NOT_FOUND;
    }
    raw->width = 0;
    raw->height = 0;

    Status status = STATUS_OK;
    int number = 0;
    int blank_seen = 0;
    char line[LEVEL_LINE_SIZE];
    while (status == STATUS_OK && fgets(line, (int)sizeof line, file) != NULL) {
        int has_newline = strchr(line, '\n') != NULL;
        ++number;
        /* Строка не поместилась в буфер — значит, она шире любой допустимой карты. */
        if (!has_newline && !feof(file)) {
            status = STATUS_BAD_SIZE;
        } else {
            int length = strip_line_end(line);
            if (length == 0) {
                /* Пустые строки в конце файла — обычное дело, их пропускаем.
                   А вот строка карты после пустой — уже ошибка формата. */
                blank_seen = 1;
            } else if (blank_seen) {
                status = STATUS_BAD_FORMAT;
            } else if (length > MAP_MAX_WIDTH) {
                status = STATUS_BAD_SIZE;
            } else if (raw->height == MAP_MAX_HEIGHT) {
                status = STATUS_BAD_SIZE;
            } else if (raw->height > 0 && length != raw->width) {
                status = STATUS_BAD_FORMAT; /* строки разной длины */
            } else {
                raw->width = length;
                memcpy(raw->rows[raw->height], line, (size_t)length);
                ++raw->height;
            }
        }
    }
    /* Ошибка чтения отличается от конца файла только флагом потока. */
    if (status == STATUS_OK && ferror(file)) {
        status = STATUS_BAD_FORMAT;
    }
    fclose(file); /* файл открыт на чтение: потерять при закрытии нечего */

    if (status != STATUS_OK) {
        *out_line = number;
        return status;
    }
    /* Пустой файл и слишком маленькая карта — недопустимый размер. */
    if (raw->height < MAP_MIN_SIDE || raw->width < MAP_MIN_SIDE) {
        return STATUS_BAD_SIZE;
    }
    return STATUS_OK;
}

/* Переносит сырые строки в карту и список монстров.
   Ищет героя: он должен быть ровно один. */
static Status build(const RawLevel *raw, Map *map, MonsterList *monsters,
                    int *out_x, int *out_y, int *out_line) {
    int heroes = 0;
    for (int y = 0; y < raw->height; ++y) {
        for (int x = 0; x < raw->width; ++x) {
            char c = raw->rows[y][x];
            if (c == '@') {
                ++heroes;
                *out_x = x;
                *out_y = y;
                c = TILE_FLOOR;
            } else if (c == 'M') {
                if (monsters_add(monsters, x, y, MONSTER_HP) != STATUS_OK) {
                    return STATUS_NO_MEMORY;
                }
                c = TILE_FLOOR;
            } else if (!map_is_tile(c)) {
                *out_line = y + 1;
                return STATUS_BAD_FORMAT; /* чужой символ */
            }
            map_set(map, x, y, c);
        }
    }
    if (heroes != 1) {
        return STATUS_BAD_FORMAT;
    }
    return STATUS_OK;
}

Status level_load(const char *path, Game *game, int *out_line) {
    if (path == NULL || game == NULL) {
        return STATUS_NOT_FOUND;
    }
    RawLevel raw; /* сырые строки весят семь килобайт — для стека это немного */
    Map map = { NULL, 0, 0 };
    MonsterList monsters;
    monsters_init(&monsters);
    int start_x = 0;
    int start_y = 0;
    int line = 0;

    /* Каждый шаг выполняется, только если предыдущие удались,
       а освобождение при неудаче одно на все ветки. */
    Status status = read_raw(path, &raw, &line);
    if (status == STATUS_OK) {
        status = map_create(&map, raw.width, raw.height);
    }
    if (status == STATUS_OK) {
        status = build(&raw, &map, &monsters, &start_x, &start_y, &line);
    }

    if (status != STATUS_OK) {
        /* Неудача: всё, что успели выделить, освобождаем, игру не трогаем. */
        map_destroy(&map);
        monsters_destroy(&monsters);
        if (out_line != NULL) {
            *out_line = line;
        }
        return status;
    }

    /* Успех: прежние карта и монстры освобождаются, на их место — новые.
       Структуры копируются присваиванием: переезжают указатели, а не клетки. */
    map_destroy(&game->map);
    monsters_destroy(&game->monsters);
    game->map = map;
    game->monsters = monsters;
    game->hero.x = start_x;
    game->hero.y = start_y;
    if (out_line != NULL) {
        *out_line = 0;
    }
    return STATUS_OK;
}
