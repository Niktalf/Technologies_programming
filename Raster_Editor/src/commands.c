#include <ctype.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "commands.h"
#include "fill.h"
#include "pixel.h"

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

/* Разбирает целое число. Возвращает 1, если в слове число и только оно. */
static int parse_int(const char *word, int *out) {
    char *end = NULL;
    long value = strtol(word, &end, 10);
    if (end == word || *end != '\0') {
        return 0;
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
    printf("  fill <x> <y> <r> <g> <b> - fill an area with a color\n");
    printf("  filltest                 - fill on checker, gradient and a plain image\n");
    printf("  save <file>              - write to PPM\n");
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
    printf("Image created.\n");
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
    int delta = 0;
    if (words->count < 2 || !parse_int(words->word[1], &delta)) {
        printf("Specify a number, e.g. bright 40\n");
        return;
    }
    image_brightness(&editor->image, delta);
    printf("Brightness changed by %d.\n", delta);
}

/* Печатает итог заливки. */
static void print_fill_report(FillReport report) {
    printf("Pixels filled: %d, max recursion depth: %d\n", report.filled, report.max_depth);
    if (report.filled == 0) {
        printf("Nothing changed: the point already has this color.\n");
    }
    if (report.truncated) {
        printf("Fill stopped: the area is too large for recursion (depth limit %d).\n",
               FILL_MAX_DEPTH);
        printf("Part of the area is not filled.\n");
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
    Pixel color = pixel_pack(channel_clamp(values[2]), channel_clamp(values[3]),
                             channel_clamp(values[4]));
    print_fill_report(fill_region(&editor->image, x, y, color));
}

/* Отдельное изображение для опыта filltest, чтобы не портить изображение
   пользователя. Статическое: оно весит 256 КБ. */
static Image test_image;

/* Задания 4 и 5: заливка из угла (0, 0) красным на трёх картинках.
   Шахматка: залита одна клетка — соседняя клетка другого цвета, и заливка
   через неё не проходит. Градиент: залит один столбец — у каждого столбца
   свой оттенок серого, и соседний столбец для заливки уже «чужой» цвет.
   Одноцветная картинка: область — все 65 536 пикселей, и глубина рекурсии
   упирается в ограничение. */
static void do_filltest(void) {
    Pixel red = pixel_pack(255, 0, 0);

    printf("Checker, cell 32:\n");
    image_init(&test_image);
    image_checker(&test_image, 32);
    print_fill_report(fill_region(&test_image, 0, 0, red));

    printf("Gradient:\n");
    image_init(&test_image);
    image_gradient(&test_image);
    print_fill_report(fill_region(&test_image, 0, 0, red));

    printf("Plain black image %d x %d:\n", IMAGE_WIDTH, IMAGE_HEIGHT);
    image_init(&test_image);
    print_fill_report(fill_region(&test_image, 0, 0, red));
}

static void do_save(Editor *editor, const Words *words) {
    if (!require_image(editor)) {
        return;
    }
    /* Без аргумента изображение пишется в файл, заданный при запуске. */
    const char *path = words->count >= 2 ? words->word[1] : editor->path;
    if (path[0] == '\0') {
        printf("Specify the file name: save <file>\n");
        return;
    }
    if (!image_save_ppm(&editor->image, path)) {
        printf("Cannot write the file: %s\n", path);
        return;
    }
    editor_set_path(editor, path);
    printf("Written: %s\n", path);
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
    } else if (is_command(name, "fill")) {
        do_fill(editor, words);
    } else if (is_command(name, "filltest")) {
        do_filltest();
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
