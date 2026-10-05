#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include "commands.h"
#include "fill.h"
#include "pixel.h"

static int is_command(const char *word, const char *name) {
    size_t i;

    for (i = 0; word[i] != '\0' && name[i] != '\0'; ++i) {
        if (tolower((unsigned char)word[i]) != name[i]) {
            return 0;
        }
    }
    return word[i] == '\0' && name[i] == '\0';
}

static int parse_int(const char *word, int *out) {
    char *end = NULL;
    const long value = strtol(word, &end, 10);

    if (end == word || *end != '\0') {
        return 0;
    }
    *out = (int)value;
    return 1;
}

void commands_print_help() {
    printf("Available operations:\n");
    printf("  info                      - information about the image\n");
    printf("  gen gradient              - gradient from black to white\n");
    printf("  gen checker [cell]        - checkerboard, default cell is 32\n");
    printf("  gen stripes [number]      - colored stripes, default is 8\n");
    printf("  invert                    - inversion\n");
    printf("  bright <number>           - brightness, for example bright 40 or bright -40\n");
    printf("  fill <x> <y> <r> <g> <b>  - fill the area with color\n");
    printf("  save <file>               - save to PPM\n");
    printf("  minmax                    - the darkest and lightest point\n");
    printf("  walk                      - inversion by passing the pointer\n");
    printf("  demo                      - copy vs address: transfer price\n");
    printf("  colors                    - check color operations\n");
    printf("  help                      - this list\n");
    printf("  quit                      - exit\n");
}

static int require_image(const Editor *editor) {
    if (!editor->loaded) {
        printf("There is no image. Create it using the command 'gen'.\n");
        return 0;
    }
    return 1;
}

static void do_gen(Editor *editor, const Words *words) {
    int parameter = 0;

    if (words->count < 2) {
        printf("Specify the type: gradient, checker, or stripes\n");
        return;
    }
    if (words->count >= 3 && !parse_int(words->word[2], &parameter)) {
        printf("Not a number: %s\n", words->word[2]);
        return;
    }

    if (is_command(words->word[1], "gradient")) {
        image_gradient(&editor->image);
    } else if (is_command(words->word[1], "checker")) {
        image_checker(&editor->image, words->count >= 3 ? parameter : 32);
    } else if (is_command(words->word[1], "stripes")) {
        image_stripes(&editor->image, words->count >= 3 ? parameter : 8);
    } else {
        printf("Unknown type: %s\n", words->word[1]);
        return;
    }
    editor->loaded = 1;
    printf("The image has been created.\n");
}

static void do_bright(Editor *editor, const Words *words) {
    int delta;

    if (!require_image(editor)) {
        return;
    }
    if (words->count < 2 || !parse_int(words->word[1], &delta)) {
        printf("Enter a number, for example, bright 40\n");
        return;
    }
    image_brightness(&editor->image, delta);
    printf("Brightness changed to %d.\n", delta);
}

static void do_save(Editor *editor, const Words *words) {
    if (!require_image(editor)) {
        return;
    }
    const char *path = words->count >= 2 ? words->word[1] : editor->path;
    if (path[0] == '\0') {
        printf("Specify the file name: save <file>\n");
        return;
    }
    if (!image_save_ppm(&editor->image, path)) {
        printf("Failed to write file: %s\n", path);
        return;
    }
    editor_set_path(editor, path);
    printf("Recorded: %s\n", path);
}

static void do_fill(Editor *editor, const Words *words) {
    if (!require_image(editor)) {
        return;
    }
    if (words->count < 6) {
        printf("You need five numbers: fill <x> <y> <r> <g> <b>\n");
        return;
    }

    int values[5];
    for (int i = 0; i < 5; ++i) {
        if (!parse_int(words->word[i + 1], &values[i])) {
            printf("Not a number: %s\n", words->word[i + 1]);
            return;
        }
    }
    if (!image_inside(&editor->image, values[0], values[1])) {
        printf("Point (%d, %d) in out image\n", values[0], values[1]);
        return;
    }

    const FillReport report = fill_region(&editor->image, values[0], values[1],
        pixel_pack(channel_clamp(values[2]), channel_clamp(values[3]), channel_clamp(values[4])));

    printf("Pixels recolored: %d, maximum recursion depth: %d\n",
           report.filled, report.max_depth);
    if (report.filled == 0) {
        printf("Nothing has changed: the dot is already this color\n");
    }
    if (report.truncated) {
        printf("Filling stopped: area is too large for recursion "
               "(depth limit %d). Part of the area has not been repainted.\n", FILL_MAX_DEPTH);
    }
}

static void invert_copy(Image image) {
    pixels_invert(image.pixels, image.width * image.height);
}

static void invert_pointer(Image *image)
{
    pixels_invert(image->pixels, image->width * image->height);
}

static void demo_copy_vs_pointer(Editor *editor) {
    Pixel before = image_get(&editor->image, 0, 0);

    invert_copy(editor->image);
    printf("After the function with a copy: pixel 0x%06X (was 0x%06X)\n",
           (unsigned)image_get(&editor->image, 0, 0), (unsigned)before);

    invert_pointer(&editor->image);
    printf("After the function with the address: pixel 0x%06X (was 0x%06X)\n",
           (unsigned)image_get(&editor->image, 0, 0), (unsigned)before);
    invert_pointer(&editor->image);

    printf("Image size: %u bytes, pointer size: %u bytes\n",
           (unsigned)sizeof(Image), (unsigned)sizeof(Image *));
    printf("A function with a copy gets a quarter of a megabyte and only changes it.\n");
    printf("It's not always possible to measure the price of this copy: after seeing\n");
    printf("that no one needs a copy, the compiler has the right not to make it.\n");
}

static void do_minmax(const Editor *editor) {
    int low = 0;
    int high = 0;

    if (image_min_max(&editor->image, &low, &high)) {
        printf("Darkest point: %d, lightest: %d\n", low, high);
    } else {
        printf("The image is empty.\n");
    }
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
        if (require_image(editor)) {
            image_invert(&editor->image);
            printf("Inverted.\n");
        }
    } else if (is_command(name, "bright")) {
        do_bright(editor, words);
    } else if (is_command(name, "fill")) {
        do_fill(editor, words);
    } else if (is_command(name, "save")) {
        do_save(editor, words);
    } else if (is_command(name, "minmax")) {
        if (require_image(editor)) {
            do_minmax(editor);
        }
    } else if (is_command(name, "walk")) {
        if (require_image(editor)) {
            int total = editor->image.width * editor->image.height;

            pixels_invert_walk(editor->image.pixels, editor->image.pixels + total);
            printf("Inverted by a pointer pass.\n");
        }
    } else if (is_command(name, "demo")) {
        if (require_image(editor)) {
            demo_copy_vs_pointer(editor);
        }
    } else if (is_command(name, "colors")) {
        commands_demo_colors();
    } else if (is_command(name, "help")) {
        commands_print_help();
    } else if (is_command(name, "quit") || is_command(name, "exit")) {
        return 0;
    } else {
        printf("Unknown operation: %s\n", name);
        printf("Type 'help' to see the list of operations.\n");
    }
    return 1;
}

void commands_demo_colors() {
    static const Pixel SAMPLES[] = {
        0x000000u,
        0xFFFFFFu,
        0xFF0000u,
        0x3C78B4u
    };
    const size_t count = sizeof SAMPLES / sizeof SAMPLES[0];

    printf("%-10s %-14s %-10s %-10s %-10s %s\n",
           "original", "channels", "lighter", "darker", "inversion", "gray");

    for (size_t i = 0; i < count; ++i) {
        const Pixel p = SAMPLES[i];
        char channels[14];

        snprintf(channels, sizeof channels, "%3u %3u %3u",
                 (unsigned)pixel_red(p), (unsigned)pixel_green(p), (unsigned)pixel_blue(p));

        printf("0x%06X %-14s 0x%06X 0x%06X 0x%06X 0x%06X\n",
               (unsigned)p, channels,
               (unsigned)pixel_adjust_brightness(p, 40),
               (unsigned)pixel_adjust_brightness(p, -40),
               (unsigned)pixel_invert(p),
               (unsigned)pixel_to_gray(p));
    }

    const Pixel p = pixel_pack(17, 200, 255);
    printf("\nPackaging reversibility: %u %u %u -> 0x%06X -> %u %u %u\n",
        17u, 200u, 255u, (unsigned)p, (unsigned)pixel_red(p), (unsigned)pixel_green(p), (unsigned)pixel_blue(p));
}
