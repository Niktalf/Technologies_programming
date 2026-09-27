#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "commands.h"
#include "pixel.h"

Operation operation_from_word(const char *word)
{
    if (word == NULL || word[0] == '\0') {
        return OP_EMPTY;
    }
    if (strcmp(word, "info") == 0) {
        return OP_INFO;
    }
    if (strcmp(word, "colors") == 0) {
        return OP_COLORS;
    }
    if (strcmp(word, "help") == 0) {
        return OP_HELP;
    }
    if (strcmp(word, "quit") == 0 || strcmp(word, "exit") == 0) {
        return OP_QUIT;
    }
    return OP_UNKNOWN;
}

void commands_print_help()
{
    printf("Available operations:\n");
    printf("  info      - information about the current image\n");
    printf("  colors    - checking color operations\n");
    printf("  help      - this list\n");
    printf("  quit      - exit\n");
}

int operation_execute(const Operation operation, const Editor *editor, const char *raw_word)
{
    switch (operation) {
    case OP_INFO:
        editor_print_info(editor);
        return 1;
    case OP_COLORS:
        commands_demo_colors();
        return 1;
    case OP_HELP:
        commands_print_help();
        return 1;
    case OP_EMPTY:
        return 1;
    case OP_QUIT:
    case OP_EOF:
        return 0;
    case OP_UNKNOWN:
    default:
        fprintf(stderr, "Unknown operation: %s\n", raw_word != NULL ? raw_word : "");
        fprintf(stderr, "Type help to see the list of operations.\n");
        return 1;
    }
}

void commands_demo_colors(void)
{
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

    {
        const Pixel p = pixel_pack(17, 200, 255);
        printf("\nPackaging reversibility: %u %u %u -> 0x%06X -> %u %u %u\n",
               17u, 200u, 255u, (unsigned)p,
               (unsigned)pixel_red(p), (unsigned)pixel_green(p), (unsigned)pixel_blue(p));
    }
}
