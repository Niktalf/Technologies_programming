#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "commands.h"
#include "pixel.h"

Operation operation_from_word(const char *word) {
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

void commands_print_help(void) {
    printf("Available operations:\n");
    printf("  info    - information about the current image\n");
    printf("  colors  - check the color operations\n");
    printf("  help    - this list\n");
    printf("  quit    - exit\n");
}

int operation_execute(Operation operation, Editor *editor, const char *word) {
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
        return 0;
    case OP_UNKNOWN:
    default:
        /* Неизвестная операция работу не завершает: печатаем подсказку и ждём дальше. */
        printf("Unknown operation: %s\n", word != NULL ? word : "");
        printf("Type help to see the list of operations.\n");
        return 1;
    }
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
