#include <ctype.h>
#include <stdio.h>

#include "ppm.h"

/* Больше этого числа в заголовке и данных PPM не бывает: стороны не больше
   IMAGE_MAX_SIDE, максимум канала — 65535. */
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
    /* Символ после числа не наш: возвращаем его в поток. В P6 это тот самый
       единственный пробел перед байтами данных. */
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
static ImgStatus read_header(FILE *file, PpmFormat *format, int *width, int *height,
                             int *maximum) {
    skip_spaces_and_comments(file);
    char signature[3];
    if (fscanf(file, "%2s", signature) != 1 || signature[0] != 'P'
        || (signature[1] != '3' && signature[1] != '6')) {
        return IMG_EFORMAT;
    }
    *format = signature[1] == '6' ? PPM_BINARY : PPM_TEXT;
    if (!read_number(file, width) || !read_number(file, height) || !read_number(file, maximum)) {
        return IMG_EFORMAT;
    }
    if (*width <= 0 || *height <= 0 || *maximum <= 0 || *maximum > 65535) {
        return IMG_EFORMAT;
    }
    /* В двоичной разновидности при максимуме больше 255 канал занимает
       два байта. Такие файлы редки, и мы их честно не поддерживаем. */
    if (*format == PPM_BINARY && *maximum > CHANNEL_MAX) {
        return IMG_EFORMAT;
    }
    /* Память выделяется под прочитанные размеры, но только после того,
       как они проверены: иначе испорченный заголовок попросит гигабайты. */
    if (*width > IMAGE_MAX_SIDE || *height > IMAGE_MAX_SIDE) {
        return IMG_EFORMAT;
    }
    /* В двоичном файле после максимума идёт ровно один пробельный символ,
       а дальше сразу байты. Пропускать пробелы дальше нельзя: первый байт
       данных может совпасть с кодом пробела. */
    if (*format == PPM_BINARY && fgetc(file) == EOF) {
        return IMG_ECORRUPT;
    }
    return IMG_OK;
}

/* Читает три канала одного пикселя текстом или байтами.
   Возвращает 0, если файл оборвался. */
static int read_channels(FILE *file, PpmFormat format, int channels[3]) {
    if (format == PPM_TEXT) {
        return read_number(file, &channels[0]) && read_number(file, &channels[1])
               && read_number(file, &channels[2]);
    }
    unsigned char bytes[3];
    if (fread(bytes, 1, 3, file) != 3) {
        return 0;
    }
    for (int i = 0; i < 3; ++i) {
        channels[i] = bytes[i];
    }
    return 1;
}

/* Приводит канал к диапазону 0..CHANNEL_MAX. Умножение до деления:
   иначе всё станет нулём. */
static uint8_t scale_channel(int value, int maximum) {
    if (maximum != CHANNEL_MAX) {
        value = value * CHANNEL_MAX / maximum;
    }
    return channel_clamp(value);
}

/* Читает пиксели в image, размеры которого уже выставлены. */
static ImgStatus read_pixels(FILE *file, PpmFormat format, int maximum, Image *image) {
    for (int y = 0; y < image->height; ++y) {
        for (int x = 0; x < image->width; ++x) {
            int c[3];
            /* Файл оборван или значение больше объявленного максимума. */
            if (!read_channels(file, format, c)
                || c[0] > maximum || c[1] > maximum || c[2] > maximum) {
                return IMG_ECORRUPT;
            }
            Color color = { scale_channel(c[0], maximum), scale_channel(c[1], maximum),
                            scale_channel(c[2], maximum) };
            image_set(image, x, y, color_to_pixel(&color));
        }
    }
    return IMG_OK;
}

/* Файл открывается и закрывается здесь, в одном месте; всё чтение —
   во вспомогательных функциях, которые на ошибке просто возвращают код.

   Читаем во временное изображение, а в целевое переносим только при успехе:
   оборванный файл не должен портить то, что уже открыто. */
ImgStatus ppm_load(Image *image, const char *path, PpmFormat *out_format) {
    if (image == NULL || path == NULL) {
        return IMG_EARG;
    }
    /* Двоичный режим всегда: для P6 он обязателен, потому что в текстовом
       некоторые системы портят байты, совпадающие с кодом перевода строки.
       Для P3 он безвреден. */
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        return IMG_EOPEN;
    }

    PpmFormat format = PPM_TEXT;
    int width = 0;
    int height = 0;
    int maximum = 0;
    Image loaded = { NULL, 0, 0 };
    ImgStatus status = read_header(file, &format, &width, &height, &maximum);
    if (status == IMG_OK) {
        status = image_create(&loaded, width, height);
    }
    if (status == IMG_OK) {
        status = read_pixels(file, format, maximum, &loaded);
    }
    fclose(file); /* файл открыт на чтение: потерять при закрытии нечего */

    if (status != IMG_OK) {
        image_destroy(&loaded); /* за собой убираем и при ошибке */
        return status;
    }
    image_destroy(image); /* прежнее изображение освобождается */
    *image = loaded;
    if (out_format != NULL) {
        *out_format = format;
    }
    return IMG_OK;
}

/* Пишет один пиксель текстом или тремя байтами. Возвращает 1 при успехе. */
static int write_pixel(FILE *file, PpmFormat format, Pixel pixel) {
    Color c = pixel_to_color(pixel);
    if (format == PPM_TEXT) {
        return fprintf(file, "%u %u %u ", (unsigned)c.red, (unsigned)c.green,
                       (unsigned)c.blue) >= 0;
    }
    unsigned char bytes[3] = { c.red, c.green, c.blue };
    return fwrite(bytes, 1, 3, file) == 3;
}

ImgStatus ppm_save(const Image *image, const char *path, PpmFormat format) {
    if (image == NULL || image->pixels == NULL || path == NULL) {
        return IMG_EARG;
    }
    FILE *file = fopen(path, "wb");
    if (file == NULL) {
        return IMG_EOPEN;
    }
    /* Заголовок: сигнатура, ширина и высота, максимальное значение канала. */
    int ok = fprintf(file, "P%c\n%d %d\n%d\n", format == PPM_BINARY ? '6' : '3',
                     image->width, image->height, CHANNEL_MAX) >= 0;
    /* В тексте числа разделяются пробелами, строки изображения — переводом
       строки. Без разделителей файл не откроет ни один просмотрщик. */
    for (int y = 0; y < image->height && ok; ++y) {
        for (int x = 0; x < image->width && ok; ++x) {
            ok = write_pixel(file, format, image_get(image, x, y));
        }
        if (ok && format == PPM_TEXT) {
            ok = fputc('\n', file) != EOF;
        }
    }
    /* Часть ошибок записи видна только при закрытии: буфер уходит на диск
       именно тогда. */
    if (fclose(file) != 0 || !ok) {
        return IMG_EWRITE;
    }
    return IMG_OK;
}
