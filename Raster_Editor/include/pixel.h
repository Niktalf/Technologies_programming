#ifndef PIXEL_H
#define PIXEL_H

#include <stdint.h>

/* Практика 2: цвет как упакованное число.
   Три канала по восемь бит укладываются в одно 32-битное беззнаковое число:
   красный в битах 16–23, зелёный в 8–15, синий в 0–7.

   Тип канала — uint8_t. Не int: на канал хватает восьми бит, а int занял бы
   четыре байта и допускал бы значения, которых у канала не бывает.
   Не char: знаковость char зависит от компилятора, и яркость 200 в знаковом
   char превратилась бы в отрицательное число. Беззнаковость при этом
   требует осторожности при вычитании — см. commands_demo_colors. */

typedef uint32_t Pixel;

#define CHANNEL_MAX 255

/* Практика 8: цвет как структура из трёх каналов. Пиксель по-прежнему
   хранится упакованным — так изображение занимает меньше памяти, —
   а структура нужна там, где цвет передают и разбирают: вместо трёх
   отдельных параметров в функцию передаётся адрес одной структуры. */
typedef struct {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} Color;

/* Перевод между структурой и упакованным пикселем. */
Pixel color_to_pixel(const Color *color);
Color pixel_to_color(Pixel pixel);

/* Упаковка и извлечение. Реализованы сдвигами и масками, а не делением. */
Pixel pixel_pack(uint8_t red, uint8_t green, uint8_t blue);
uint8_t pixel_red(Pixel pixel);
uint8_t pixel_green(Pixel pixel);
uint8_t pixel_blue(Pixel pixel);

/* Изменение яркости с насыщением: результат не выходит за границы канала.
   Положительная величина осветляет, отрицательная затемняет. */
Pixel pixel_adjust_brightness(Pixel pixel, int delta);

/* Инверсия каждого канала. */
Pixel pixel_invert(Pixel pixel);

/* Перевод в оттенок серого по взвешенной формуле.
   Веса целочисленные, умножение выполняется до деления. */
uint8_t pixel_luminance(Pixel pixel);
Pixel pixel_to_gray(Pixel pixel);

/* Значение канала с насыщением: выход за границы приводится к границе. */
uint8_t channel_clamp(int value);

#endif /* PIXEL_H */
