#ifndef IMAGE_H
#define IMAGE_H

#include "pixel.h"

/* Практика 3: изображение фиксированного размера в массиве.
   Пиксели лежат одной строкой подряд: сначала вся первая строка
   изображения, затем вся вторая и так далее. Динамический размер
   появится на практике 7. */

#define IMAGE_WIDTH  256
#define IMAGE_HEIGHT 256

typedef struct {
    Pixel pixels[IMAGE_WIDTH * IMAGE_HEIGHT];
    int width;
    int height;
} Image;

/* Такой объект весит 256 КБ. Локальной переменной в функции его делать нельзя:
   стек потока обычно меньше мегабайта, а в Windows ровно мегабайт. */

void image_init(Image *image);

/* Смещение пикселя в массиве вычисляется только здесь.
   Номер строки умножается на ширину, а не на высоту. */
int image_offset(const Image *image, int x, int y);

int image_inside(const Image *image, int x, int y);
Pixel image_get(const Image *image, int x, int y);
void image_set(Image *image, int x, int y, Pixel color);

/* Тестовые изображения. */
void image_gradient(Image *image);
void image_checker(Image *image, int cell);
void image_stripes(Image *image, int count);

/* Фильтры по всему изображению. */
void image_invert(Image *image);
void image_brightness(Image *image, int delta);

/* Записывает изображение в текстовый формат PPM (P3).
   Возвращает 1 при успехе и 0, если файл не открылся. */
int image_save_ppm(const Image *image, const char *path);

#endif /* IMAGE_H */
