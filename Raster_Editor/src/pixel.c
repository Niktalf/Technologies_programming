#include <stddef.h>

#include "pixel.h"

#define RED_SHIFT    16
#define GREEN_SHIFT  8
#define BLUE_SHIFT   0
#define CHANNEL_MASK 0xFFu /* восемь младших битов: ровно один канал */

uint8_t channel_clamp(int value) {
    if (value < 0) {
        return 0;
    }
    if (value > CHANNEL_MAX) {
        return (uint8_t)CHANNEL_MAX;
    }
    return (uint8_t)value;
}

Pixel pixel_pack(uint8_t red, uint8_t green, uint8_t blue) {
    /* Канал приводится к Pixel до сдвига: сдвигать восьмибитное значение
       бессмысленно, а после целочисленного повышения оно стало бы знаковым int. */
    return ((Pixel)red << RED_SHIFT) | ((Pixel)green << GREEN_SHIFT)
           | ((Pixel)blue << BLUE_SHIFT);
}

uint8_t pixel_red(Pixel pixel) {
    return (uint8_t)((pixel >> RED_SHIFT) & CHANNEL_MASK);
}

uint8_t pixel_green(Pixel pixel) {
    return (uint8_t)((pixel >> GREEN_SHIFT) & CHANNEL_MASK);
}

uint8_t pixel_blue(Pixel pixel) {
    return (uint8_t)((pixel >> BLUE_SHIFT) & CHANNEL_MASK);
}

Pixel pixel_adjust_brightness(Pixel pixel, int delta) {
    /* Вычисления идут в int: в узком типе проверять насыщение уже поздно. */
    int red = (int)pixel_red(pixel) + delta;
    int green = (int)pixel_green(pixel) + delta;
    int blue = (int)pixel_blue(pixel) + delta;
    return pixel_pack(channel_clamp(red), channel_clamp(green), channel_clamp(blue));
}

Pixel pixel_invert(Pixel pixel) {
    return pixel_pack((uint8_t)(CHANNEL_MAX - pixel_red(pixel)),
                      (uint8_t)(CHANNEL_MAX - pixel_green(pixel)),
                      (uint8_t)(CHANNEL_MAX - pixel_blue(pixel)));
}

uint8_t pixel_luminance(Pixel pixel) {
    /* Веса 0,299, 0,587 и 0,114 взяты целыми числами с общим делителем 1000.
       Умножение выполняется до деления, иначе целочисленное деление
       обнулит вклад каждого канала по отдельности. */
    int value = (299 * (int)pixel_red(pixel) + 587 * (int)pixel_green(pixel)
                 + 114 * (int)pixel_blue(pixel)) / 1000;
    return channel_clamp(value);
}

Pixel pixel_to_gray(Pixel pixel) {
    uint8_t gray = pixel_luminance(pixel);
    return pixel_pack(gray, gray, gray);
}

Pixel color_to_pixel(const Color *color) {
    if (color == NULL) {
        return pixel_pack(0, 0, 0);
    }
    return pixel_pack(color->red, color->green, color->blue);
}

/* Структура из трёх байтов возвращается по значению: копировать её дёшево. */
Color pixel_to_color(Pixel pixel) {
    Color color = { pixel_red(pixel), pixel_green(pixel), pixel_blue(pixel) };
    return color;
}
