#ifndef PIXEL_H
#define PIXEL_H

#include <stdint.h>

typedef uint32_t Pixel;

#define CHANNEL_MAX 255

Pixel   pixel_pack(uint8_t red, uint8_t green, uint8_t blue);
uint8_t pixel_red(Pixel pixel);
uint8_t pixel_green(Pixel pixel);
uint8_t pixel_blue(Pixel pixel);

Pixel pixel_adjust_brightness(Pixel pixel, int delta);
Pixel pixel_invert(Pixel pixel);
uint8_t pixel_luminance(Pixel pixel);
Pixel   pixel_to_gray(Pixel pixel);

uint8_t channel_clamp(int value);

#endif // PIXEL_H
