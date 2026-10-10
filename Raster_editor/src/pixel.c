#include "pixel.h"

#define RED_SHIFT   16
#define GREEN_SHIFT 8
#define BLUE_SHIFT  0
#define CHANNEL_MASK 0xFFu

uint8_t channel_clamp(const int value) {
    if (value < 0) {
        return 0;
    }
    if (value > CHANNEL_MAX) {
        return (uint8_t)CHANNEL_MAX;
    }
    return (uint8_t)value;
}

Pixel pixel_pack(const uint8_t red, const uint8_t green, const uint8_t blue) {
    return ((Pixel)red   << RED_SHIFT)
         | ((Pixel)green << GREEN_SHIFT)
         | ((Pixel)blue  << BLUE_SHIFT);
}

uint8_t pixel_red(const Pixel pixel) {
    return (uint8_t)((pixel >> RED_SHIFT) & CHANNEL_MASK);
}

uint8_t pixel_green(const Pixel pixel) {
    return (uint8_t)((pixel >> GREEN_SHIFT) & CHANNEL_MASK);
}

uint8_t pixel_blue(const Pixel pixel) {
    return (uint8_t)((pixel >> BLUE_SHIFT) & CHANNEL_MASK);
}

Pixel pixel_adjust_brightness(const Pixel pixel, const int delta) {
    const int red   = (int)pixel_red(pixel)   + delta;
    const int green = (int)pixel_green(pixel) + delta;
    const int blue  = (int)pixel_blue(pixel)  + delta;

    return pixel_pack(channel_clamp(red), channel_clamp(green), channel_clamp(blue));
}

Pixel pixel_invert(const Pixel pixel) {
    return pixel_pack((uint8_t)(CHANNEL_MAX - pixel_red(pixel)),
                      (uint8_t)(CHANNEL_MAX - pixel_green(pixel)),
                      (uint8_t)(CHANNEL_MAX - pixel_blue(pixel)));
}

uint8_t pixel_luminance(const Pixel pixel) {
    const int value = (299 * (int)pixel_red(pixel)
        + 587 * (int)pixel_green(pixel)
        + 114 * (int)pixel_blue(pixel)) / 1000;
    return channel_clamp(value);
}

Pixel pixel_to_gray(const Pixel pixel) {
    const uint8_t gray = pixel_luminance(pixel);
    return pixel_pack(gray, gray, gray);
}
