#include <ctype.h>
#include <stdio.h>

#include "ppm.h"

const char *ppm_status_text(PpmStatus status) {
    switch (status) {
    case PPM_OK:
        return "success";
    case PPM_ERR_OPEN:
        return "cannot open the file";
    case PPM_ERR_SIGNATURE:
        return "not a P3 file";
    case PPM_ERR_HEADER:
        return "corrupted header";
    case PPM_ERR_SIZE:
        return "the image is too large";
    case PPM_ERR_DATA:
        return "corrupted data: the file is cut off or a value exceeds the maximum";
    default:
        return "unknown error";
    }
}

/* Больше этого числа в заголовке и данных PPM не бывает: стороны не больше
   IMAGE_WIDTH и IMAGE_HEIGHT, максимум канала — 65535. */
#define PPM_NUMBER_MAX 1000000

/* Пропускает пробелы и строки комментариев.
   Символ, который оказался не пробелом и не решёткой, возвращается
   обратно в поток: читать его будет уже тот, кому он нужен. */
static void skip_spaces_and_comments(FILE *file) {
    int c = fgetc(file);
    while (c == '#' || (c != EOF && isspace(c))) {
        if (c == '#') {
            /* Комментарий — до конца строки. */
            while (c != EOF && c != '\n') {
                c = fgetc(file);
            }
        }
        c = fgetc(file);
    }
    if (c != EOF) {
        ungetc(c, file);
    }
}

/* Читает одно неотрицательное целое: пропускает пробелы и комментарии,
   затем собирает цифры сам. fscanf("%d") здесь не годится: на числе,
   которое не помещается в int, его поведение не определено, а файл
   с шириной в сто миллиардов должен давать ошибку, а не что попало.
   Возвращает 1, если число прочитано и не больше PPM_NUMBER_MAX. */
static int read_number(FILE *file, int *out) {
    skip_spaces_and_comments(file);
    long value = 0;
    int digits = 0;
    int c = fgetc(file);
    while (c != EOF && isdigit(c)) {
        value = value * 10 + (c - '0');
        if (value > PPM_NUMBER_MAX) {
            return 0; /* проверяем на каждой цифре, пока long не переполнился */
        }
        ++digits;
        c = fgetc(file);
    }
    /* Символ после числа не наш: возвращаем его в поток. */
    if (c != EOF) {
        ungetc(c, file);
    }
    if (digits == 0) {
        return 0;
    }
    *out = (int)value;
    return 1;
}

/* Читает и проверяет заголовок: сигнатуру, размеры и максимум канала. */
static PpmStatus read_header(FILE *file, int *width, int *height, int *maximum) {
    skip_spaces_and_comments(file);
    char signature[3];
    if (fscanf(file, "%2s", signature) != 1 || signature[0] != 'P' || signature[1] != '3') {
        return PPM_ERR_SIGNATURE; /* двоичный P6 появится на практике 8 */
    }
    if (!read_number(file, width) || !read_number(file, height) || !read_number(file, maximum)) {
        return PPM_ERR_HEADER;
    }
    if (*width <= 0 || *height <= 0 || *maximum <= 0 || *maximum > 65535) {
        return PPM_ERR_HEADER;
    }
    /* Размер нашего изображения фиксирован: больший файл пока не прочитать.
       Изображение любого размера появится на практике 7. */
    if (*width > IMAGE_WIDTH || *height > IMAGE_HEIGHT) {
        return PPM_ERR_SIZE;
    }
    return PPM_OK;
}

/* Читает один канал и приводит его к диапазону 0..CHANNEL_MAX.
   Значение больше объявленного максимума — испорченные данные. */
static int read_channel(FILE *file, int maximum, uint8_t *out) {
    int value = 0;
    if (!read_number(file, &value) || value > maximum) {
        return 0;
    }
    /* Если максимум не 255, значение масштабируется.
       Умножение до деления: иначе всё станет нулём. */
    if (maximum != CHANNEL_MAX) {
        value = value * CHANNEL_MAX / maximum;
    }
    *out = channel_clamp(value);
    return 1;
}

/* Читает пиксели в image, размеры которого уже выставлены. */
static PpmStatus read_pixels(FILE *file, Image *image, int maximum) {
    for (int y = 0; y < image->height; ++y) {
        for (int x = 0; x < image->width; ++x) {
            uint8_t red = 0;
            uint8_t green = 0;
            uint8_t blue = 0;
            /* Файл оборван на середине или значение больше максимума. */
            if (!read_channel(file, maximum, &red) || !read_channel(file, maximum, &green)
                || !read_channel(file, maximum, &blue)) {
                return PPM_ERR_DATA;
            }
            image_set(image, x, y, pixel_pack(red, green, blue));
        }
    }
    return PPM_OK;
}

/* Читаем во временное изображение, а в целевое переносим только при успехе.
   Иначе размеры пришлось бы выставлять до чтения — а по ним считается
   смещение пикселя, и при ошибке в целевом изображении осталась бы каша.
   Временное изображение статическое: четверть мегабайта в стек не положить. */
static Image scratch;

/* Файл открывается и закрывается здесь, в одном месте; всё чтение —
   во вспомогательных функциях, которые на ошибке просто возвращают код.
   Так fclose не приходится повторять перед каждым return. */
PpmStatus ppm_load(Image *image, const char *path) {
    FILE *file = fopen(path, "r");
    if (file == NULL) {
        return PPM_ERR_OPEN;
    }

    int width = 0;
    int height = 0;
    int maximum = 0;
    PpmStatus status = read_header(file, &width, &height, &maximum);
    if (status == PPM_OK) {
        /* Размеры выставляются до чтения пикселей: по ним считается смещение. */
        image_init(&scratch);
        scratch.width = width;
        scratch.height = height;
        status = read_pixels(file, &scratch, maximum);
    }
    fclose(file);

    if (status == PPM_OK) {
        *image = scratch; /* целевое изображение меняется только при успехе */
    }
    return status;
}
