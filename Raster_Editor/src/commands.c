#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "commands.h"
#include "fill.h"
#include "pixel.h"
#include "ppm.h"

/* Сравнивает имя команды без учёта регистра.
   В интерактивном режиме имя уже приведено к нижнему регистру,
   а аргументы командной строки менять нельзя — поэтому сравниваем здесь. */
static int is_command(const char *word, const char *name) {
    size_t i = 0;
    while (name[i] != '\0' && tolower((unsigned char)word[i]) == name[i]) {
        ++i;
    }
    /* Совпадение — только если оба слова кончились одновременно. */
    return word[i] == '\0' && name[i] == '\0';
}

/* Разбирает целое число. Возвращает 1, если в слове число и только оно
   и оно помещается в int. */
static int parse_int(const char *word, int *out) {
    /* strtol сообщает о переполнении через errno, а не возвращаемым
       значением, поэтому errno обнуляется до вызова. Знак «+» и «-»
       strtol понимает сам. */
    char *end = NULL;
    errno = 0;
    long value = strtol(word, &end, 10);
    if (end == word || *end != '\0') {
        return 0; /* пустое слово или после числа есть что-то ещё */
    }
    if (errno == ERANGE || value < INT_MIN || value > INT_MAX) {
        return 0; /* число не помещается в int */
    }
    *out = (int)value;
    return 1;
}

void commands_print_help(void) {
    printf("Available operations:\n");
    printf("  info                     - information about the image\n");
    printf("  gen gradient             - gradient from black to white\n");
    printf("  gen checker [cell]       - checkerboard, cell 32 by default\n");
    printf("  gen stripes [count]      - color stripes, 8 by default\n");
    printf("  invert                   - invert\n");
    printf("  bright <number>          - brightness, e.g. bright 40 or bright -40\n");
    printf("  size <w> <h>             - create an empty image\n");
    printf("  crop <x> <y> <w> <h>     - crop\n");
    printf("  scale <w> <h>            - resize\n");
    printf("  rotate <angle>           - rotate by a multiple of 90 degrees\n");
    printf("  fill <x> <y> <r> <g> <b> - fill an area with a color\n");
    printf("  filltest                 - fill on checker, gradient and plain images\n");
    printf("  stress <file> [n]        - load, process and free n times\n");
    printf("  minmax                   - the darkest and the brightest point\n");
    printf("  walk                     - invert by walking a pointer\n");
    printf("  demo                     - copy of the structure vs address\n");
    printf("  load <file>              - read a PPM, either P3 or P6\n");
    printf("  save <file> [text|bin]   - write, text P3 by default\n");
    printf("  colors                   - check the color operations\n");
    printf("  help                     - this list\n");
    printf("  quit                     - exit\n");
}

/* Проверяет, что изображение есть, и печатает подсказку, если его нет. */
static int require_image(const Editor *editor) {
    if (!editor->loaded) {
        printf("No image. Create one with gen.\n");
        return 0;
    }
    return 1;
}

/* Делает так, чтобы в редакторе было изображение заданного размера.
   Новое изображение создаётся раньше, чем освобождается старое: если
   размер неверен или памяти не хватило, прежнее изображение остаётся. */
static int ensure_image(Editor *editor, int width, int height) {
    if (editor->image.pixels != NULL
        && editor->image.width == width && editor->image.height == height) {
        return 1;
    }
    Image created;
    ImgStatus status = image_create(&created, width, height);
    if (status != IMG_OK) {
        printf("Cannot create an image %d x %d: %s\n", width, height, img_status_text(status));
        return 0;
    }
    image_destroy(&editor->image);
    editor->image = created;
    return 1;
}

static void do_size(Editor *editor, const Words *words) {
    int width = 0;
    int height = 0;
    if (words->count != 3 || !parse_int(words->word[1], &width)
        || !parse_int(words->word[2], &height)) {
        printf("Usage: size <width> <height>\n");
        return;
    }
    if (!ensure_image(editor, width, height)) {
        return;
    }
    editor->loaded = 1;
    printf("Image created %d x %d (%lu bytes)\n", width, height,
           (unsigned long)((size_t)width * (size_t)height * sizeof(Pixel)));
}

static void do_gen(Editor *editor, const Words *words) {
    if (words->count < 2) {
        printf("Specify the kind: gradient, checker or stripes\n");
        return;
    }
    /* Необязательный числовой параметр: размер клетки или число полос. */
    int has_parameter = words->count >= 3;
    int parameter = 0;
    if (has_parameter && !parse_int(words->word[2], &parameter)) {
        printf("Not a number: %s\n", words->word[2]);
        return;
    }
    /* Генератор рисует на изображении того размера, какой есть: размер
       задаёт команда size, load или crop. Если изображения ещё нет,
       создаётся 256 на 256. */
    if (editor->image.pixels == NULL && !ensure_image(editor, 256, 256)) {
        return;
    }

    const char *kind = words->word[1];
    if (is_command(kind, "gradient")) {
        image_gradient(&editor->image);
    } else if (is_command(kind, "checker")) {
        image_checker(&editor->image, has_parameter ? parameter : 32);
    } else if (is_command(kind, "stripes")) {
        image_stripes(&editor->image, has_parameter ? parameter : 8);
    } else {
        printf("Unknown kind: %s\n", kind);
        return;
    }
    editor->loaded = 1;
    printf("Image created %d x %d.\n", editor->image.width, editor->image.height);
}

static void do_invert(Editor *editor) {
    if (!require_image(editor)) {
        return;
    }
    image_invert(&editor->image);
    printf("Inverted.\n");
}

static void do_bright(Editor *editor, const Words *words) {
    if (!require_image(editor)) {
        return;
    }
    if (words->count < 2) {
        printf("Specify a number, e.g. bright 40\n");
        return;
    }
    if (words->count > 2) {
        printf("Too many arguments: bright takes one number\n");
        return;
    }
    int delta = 0;
    if (!parse_int(words->word[1], &delta)) {
        printf("Not a number: %s. Example: bright 40 or bright -40\n", words->word[1]);
        return;
    }
    image_brightness(&editor->image, delta);
    printf("Brightness changed by %d.\n", delta);
}

static void do_crop(Editor *editor, const Words *words) {
    if (!require_image(editor)) {
        return;
    }
    if (words->count != 5) {
        printf("Usage: crop <x> <y> <width> <height>\n");
        return;
    }
    int v[4];
    for (int i = 0; i < 4; ++i) {
        if (!parse_int(words->word[i + 1], &v[i])) {
            printf("Not a number: %s\n", words->word[i + 1]);
            return;
        }
    }
    ImgStatus status = image_crop(&editor->image, v[0], v[1], v[2], v[3]);
    if (status == IMG_EARG) {
        printf("Crop failed: the rectangle must lie inside the image.\n");
        return;
    }
    if (status != IMG_OK) {
        printf("Crop failed: %s\n", img_status_text(status));
        return;
    }
    printf("Cropped to %d x %d\n", editor->image.width, editor->image.height);
}

static void do_rotate(Editor *editor, const Words *words) {
    if (!require_image(editor)) {
        return;
    }
    if (words->count != 2) {
        printf("Usage: rotate <angle>, a multiple of 90: rotate 90, rotate -90\n");
        return;
    }
    int angle = 0;
    if (!parse_int(words->word[1], &angle)) {
        printf("Not a number: %s\n", words->word[1]);
        return;
    }
    ImgStatus status = image_rotate(&editor->image, angle);
    if (status == IMG_EARG) {
        printf("The angle must be a multiple of 90 degrees.\n");
        return;
    }
    if (status != IMG_OK) {
        printf("Rotate failed: %s\n", img_status_text(status));
        return;
    }
    printf("Rotated by %d. Size: %d x %d\n", angle, editor->image.width, editor->image.height);
}

static void do_scale(Editor *editor, const Words *words) {
    if (!require_image(editor)) {
        return;
    }
    int width = 0;
    int height = 0;
    if (words->count != 3 || !parse_int(words->word[1], &width)
        || !parse_int(words->word[2], &height)) {
        printf("Usage: scale <width> <height>\n");
        return;
    }
    ImgStatus status = image_resize(&editor->image, width, height);
    if (status != IMG_OK) {
        printf("Resize failed: %s\n", img_status_text(status));
        return;
    }
    printf("New size: %d x %d\n", editor->image.width, editor->image.height);
}

/* Задание 5: сто раз подряд загрузить файл, обработать — обрезать, увеличить,
   повернуть — и освободить. Каждая операция выделяет новый блок
   и освобождает старый. Если где-то освобождение забыто, потребление
   памяти растёт с каждым кругом, а санитайзер назовёт место утечки. */
static void do_stress(const Words *words) {
    if (words->count < 2) {
        printf("Usage: stress <file> [repeat count]\n");
        return;
    }
    const char *path = words->word[1];
    int times = 100;
    if (words->count >= 3 && !parse_int(words->word[2], &times)) {
        printf("Not a number: %s\n", words->word[2]);
        return;
    }
    int failures = 0;
    for (int i = 0; i < times; ++i) {
        Image temp = { NULL, 0, 0 };
        ImgStatus status = ppm_load(&temp, path, NULL);
        if (status != IMG_OK) {
            printf("Cannot read %s: %s\n", path, img_status_text(status));
            return;
        }
        if (image_crop(&temp, 0, 0, (temp.width + 1) / 2, (temp.height + 1) / 2) != IMG_OK
            || image_resize(&temp, temp.width * 2, temp.height * 2) != IMG_OK
            || image_rotate(&temp, 90) != IMG_OK) {
            ++failures;
        }
        image_destroy(&temp);
        image_destroy(&temp); /* повторное освобождение безопасно: указатель обнулён */
    }
    printf("Loaded, processed and freed: %d times, failures: %d\n", times - failures, failures);
}

static void do_load(Editor *editor, const Words *words) {
    if (words->count < 2) {
        printf("Specify the file name: load <file>\n");
        return;
    }
    const char *path = words->word[1];
    PpmFormat format = PPM_TEXT;
    ImgStatus status = ppm_load(&editor->image, path, &format);
    if (status != IMG_OK) {
        printf("Cannot read %s: %s\n", path, img_status_text(status));
        return;
    }
    editor->loaded = 1;
    editor_set_path(editor, path);
    printf("Read: %s (%s), size %d x %d\n", path,
           format == PPM_BINARY ? "binary P6" : "text P3",
           editor->image.width, editor->image.height);
}

/* Печатает итог заливки. */
static void print_fill_report(FillReport report) {
    printf("Pixels filled: %d, largest stack of pending points: %d\n",
           report.filled, report.max_stack);
    if (report.filled == 0) {
        printf("Nothing changed: the point already has this color.\n");
    }
    if (report.failed) {
        printf("Fill stopped: not enough memory. Part of the area is not filled.\n");
    }
}

static void do_fill(Editor *editor, const Words *words) {
    if (!require_image(editor)) {
        return;
    }
    if (words->count < 6) {
        printf("Five numbers needed: fill <x> <y> <r> <g> <b>\n");
        return;
    }
    /* Пять чисел подряд: координаты точки и три канала цвета. */
    int values[5];
    for (int i = 0; i < 5; ++i) {
        if (!parse_int(words->word[i + 1], &values[i])) {
            printf("Not a number: %s\n", words->word[i + 1]);
            return;
        }
    }
    int x = values[0];
    int y = values[1];
    if (!image_inside(&editor->image, x, y)) {
        printf("Point (%d, %d) is outside the image\n", x, y);
        return;
    }
    /* Три канала собираются в одну структуру и передаются адресом,
       а не тремя отдельными параметрами. */
    Color color = { channel_clamp(values[2]), channel_clamp(values[3]),
                    channel_clamp(values[4]) };
    print_fill_report(fill_region(&editor->image, x, y, &color));
}

/* Заливает из угла (0, 0) красным изображение заданного размера,
   нарисованное функцией draw, или чёрное, если draw равна NULL.
   Изображение отдельное, чтобы не портить изображение пользователя. */
static void fill_sample(const char *title, int width, int height, void (*draw)(Image *)) {
    printf("%s %d x %d:\n", title, width, height);
    Image sample;
    ImgStatus status = image_create(&sample, width, height);
    if (status != IMG_OK) {
        printf("%s\n", img_status_text(status));
        return;
    }
    if (draw != NULL) {
        draw(&sample);
    }
    Color red = { 255, 0, 0 };
    print_fill_report(fill_region(&sample, 0, 0, &red));
    image_destroy(&sample);
}

static void draw_checker32(Image *image) {
    image_checker(image, 32);
}

/* Шахматка: залита одна клетка — соседняя клетка другого цвета, и заливка
   через неё не проходит. Градиент: залит один столбец — у каждого столбца
   свой оттенок серого. Одноцветная картинка: на практике 4 рекурсия
   упиралась здесь в стек, теперь заливается вся, даже 2048 на 2048. */
static void do_filltest(void) {
    fill_sample("Checker, cell 32,", 256, 256, draw_checker32);
    fill_sample("Gradient", 256, 256, image_gradient);
    fill_sample("Plain black image", 256, 256, NULL);
    fill_sample("Plain black image", 2048, 2048, NULL);
}

/* Структура Image теперь — указатель на пиксели и два размера. Копия
   структуры копирует указатель, а не пиксели: «копия» смотрит на тот же
   блок памяти. Такая копия называется поверхностной. */
static void invert_copy(Image image) {
    pixels_invert(image.pixels, image.width * image.height);
}

static void invert_pointer(Image *image) {
    pixels_invert(image->pixels, image->width * image->height);
}

static void do_demo(Editor *editor) {
    if (!require_image(editor)) {
        return;
    }
    Pixel before = image_get(&editor->image, 0, 0);

    invert_copy(editor->image);
    printf("After the function taking a copy:     pixel 0x%06X (was 0x%06X)\n",
           (unsigned)image_get(&editor->image, 0, 0), (unsigned)before);
    invert_copy(editor->image); /* возвращаем как было */

    invert_pointer(&editor->image);
    printf("After the function taking an address: pixel 0x%06X (was 0x%06X)\n",
           (unsigned)image_get(&editor->image, 0, 0), (unsigned)before);
    invert_pointer(&editor->image); /* возвращаем как было */

    printf("Image structure: %u bytes, pixels: %lu bytes\n", (unsigned)sizeof(Image),
           (unsigned long)((size_t)image_pixel_count(&editor->image) * sizeof(Pixel)));
    printf("Since practicum 7 the structure holds only a pointer to the pixels:\n");
    printf("a copy of the structure points to the same pixels, so the function\n");
    printf("taking a copy changed the original image too. A real copy of the\n");
    printf("pixels needs its own block: image_copy.\n");
}

static void do_minmax(const Editor *editor) {
    if (!require_image(editor)) {
        return;
    }
    int low = 0;
    int high = 0;
    if (image_min_max(&editor->image, &low, &high) != IMG_OK) {
        printf("The image is empty.\n");
        return;
    }
    printf("Darkest point: %d, brightest: %d\n", low, high);
}

static void do_walk(Editor *editor) {
    if (!require_image(editor)) {
        return;
    }
    /* Указатель на конец — адрес сразу за последним пикселем. */
    Pixel *begin = editor->image.pixels;
    Pixel *end = begin + image_pixel_count(&editor->image);
    pixels_invert_walk(begin, end);
    printf("Inverted by walking a pointer.\n");
}

static void do_save(Editor *editor, const Words *words) {
    if (!require_image(editor)) {
        return;
    }
    /* Без аргумента изображение пишется в файл, заданный при запуске. */
    const char *path = words->count >= 2 ? words->word[1] : editor->path;
    if (path[0] == '\0') {
        printf("Specify the file name: save <file> [text|bin]\n");
        return;
    }
    PpmFormat format = PPM_TEXT;
    if (words->count >= 3) {
        if (is_command(words->word[2], "bin")) {
            format = PPM_BINARY;
        } else if (!is_command(words->word[2], "text")) {
            printf("The kind is text or bin\n");
            return;
        }
    }
    ImgStatus status = ppm_save(&editor->image, path, format);
    if (status != IMG_OK) {
        printf("Cannot write %s: %s\n", path, img_status_text(status));
        return;
    }
    editor_set_path(editor, path);
    printf("Written: %s (%s)\n", path, format == PPM_BINARY ? "binary P6" : "text P3");
}

int commands_execute(Editor *editor, const Words *words) {
    if (words->count == 0) {
        return 1;
    }
    const char *name = words->word[0];
    if (is_command(name, "info")) {
        editor_print_info(editor);
    } else if (is_command(name, "gen")) {
        do_gen(editor, words);
    } else if (is_command(name, "invert")) {
        do_invert(editor);
    } else if (is_command(name, "bright")) {
        do_bright(editor, words);
    } else if (is_command(name, "size")) {
        do_size(editor, words);
    } else if (is_command(name, "crop")) {
        do_crop(editor, words);
    } else if (is_command(name, "scale")) {
        do_scale(editor, words);
    } else if (is_command(name, "stress")) {
        do_stress(words);
    } else if (is_command(name, "rotate")) {
        do_rotate(editor, words);
    } else if (is_command(name, "fill")) {
        do_fill(editor, words);
    } else if (is_command(name, "filltest")) {
        do_filltest();
    } else if (is_command(name, "minmax")) {
        do_minmax(editor);
    } else if (is_command(name, "walk")) {
        do_walk(editor);
    } else if (is_command(name, "demo")) {
        do_demo(editor);
    } else if (is_command(name, "load")) {
        do_load(editor, words);
    } else if (is_command(name, "save")) {
        do_save(editor, words);
    } else if (is_command(name, "colors")) {
        commands_demo_colors();
    } else if (is_command(name, "help")) {
        commands_print_help();
    } else if (is_command(name, "quit") || is_command(name, "exit")) {
        return 0;
    } else {
        /* Неизвестная операция работу не завершает: печатаем подсказку и ждём дальше. */
        printf("Unknown operation: %s\n", name);
        printf("Type help to see the list of operations.\n");
    }
    return 1;
}

/* Таблица преобразований на краевых и обычном цвете. */
static void demo_table(void) {
    static const Pixel SAMPLES[] = {
        0x000000u, /* чёрный: затемнять некуда */
        0xFFFFFFu, /* белый: осветлять некуда */
        0xFF0000u, /* красный: один канал на максимуме, два на нуле */
        0x3C78B4u  /* произвольный */
    };
    const size_t count = sizeof SAMPLES / sizeof SAMPLES[0];

    printf("%-8s  %-11s  %-8s  %-8s  %-8s  %s\n",
           "original", "channels", "brighter", "darker", "inverted", "gray");
    for (size_t i = 0; i < count; ++i) {
        Pixel p = SAMPLES[i];
        char channels[16];
        snprintf(channels, sizeof channels, "%3u %3u %3u",
                 (unsigned)pixel_red(p), (unsigned)pixel_green(p), (unsigned)pixel_blue(p));
        printf("0x%06X  %-11s  0x%06X  0x%06X  0x%06X  0x%06X\n",
               (unsigned)p, channels,
               (unsigned)pixel_adjust_brightness(p, 40),
               (unsigned)pixel_adjust_brightness(p, -40),
               (unsigned)pixel_invert(p),
               (unsigned)pixel_to_gray(p));
    }
}

/* Упаковка и распаковка взаимно обратны: каналы возвращаются те же. */
static void demo_round_trip(void) {
    Pixel p = pixel_pack(17, 200, 255);
    printf("\nPacking round trip: %u %u %u -> 0x%06X -> %u %u %u\n",
           17u, 200u, 255u, (unsigned)p,
           (unsigned)pixel_red(p), (unsigned)pixel_green(p), (unsigned)pixel_blue(p));
}

/* Задание 5: что будет, если вычесть из беззнакового канала больше, чем в нём есть.
   Беззнаковая арифметика в C идёт по модулю 2 в степени числа бит: ниже нуля
   значение не уходит, а «проворачивается» к верхней границе типа. Ошибкой
   с точки зрения языка это не считается, поэтому ни компилятор, ни санитайзер
   ничего не скажут — тёмный пиксель молча станет почти белым. */
static void demo_unsigned_underflow(void) {
    printf("\nSubtracting 40 from a channel equal to 30:\n");

    /* uint8_t: 30 - 40 вычисляется в int и равно -10, но при записи обратно
       в восьмибитный тип берётся остаток по модулю 256: 256 - 10 = 246. */
    uint8_t narrow = 30;
    narrow = (uint8_t)(narrow - 40);
    printf("  uint8_t without a check:      %u\n", (unsigned)narrow);

    /* unsigned int: здесь вычитание выполняется уже в беззнаковом типе,
       и результат — 2^32 - 10. */
    unsigned int wide = 30u;
    wide -= 40u;
    printf("  unsigned int without a check: %u\n", wide);

    /* Правильно: считать в знаковом типе с запасом и насыщать результат. */
    printf("  int and saturation:           %u\n", (unsigned)channel_clamp(30 - 40));

    /* То же насыщение на верхней границе. */
    printf("Adding 40 to a channel equal to 230:\n");
    printf("  uint8_t without a check:      %u\n", (unsigned)(uint8_t)(230 + 40));
    printf("  int and saturation:           %u\n", (unsigned)channel_clamp(230 + 40));
}

void commands_demo_colors(void) {
    demo_table();
    demo_round_trip();
    demo_unsigned_underflow();
}
