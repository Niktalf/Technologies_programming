#ifndef IMAGE_H
#define IMAGE_H

#include "pixel.h"

/* Практика 7: изображение произвольного размера в динамической памяти.
   Пиксели лежат одним блоком в куче, строка за строкой. Сам объект Image —
   только указатель на блок и два размера, шестнадцать байт. */

#define IMAGE_MAX_SIDE 8192

typedef struct {
    Pixel *pixels; /* width * height элементов одним блоком; NULL — изображения нет */
    int width;
    int height;
} Image;

/* Создаёт изображение, залитое чёрным. Возвращает 1 при успехе и 0,
   если размеры неразумны, их произведение переполняется или памяти
   не хватило; в этом случае pixels равен NULL. */
int image_create(Image *image, int width, int height);

/* Освобождает блок пикселей и обнуляет указатель. Повторный вызов безопасен. */
void image_destroy(Image *image);

/* Создаёт destination как отдельную копию source со своим блоком пикселей. */
int image_copy(Image *destination, const Image *source);

/* Смещение пикселя в блоке вычисляется только здесь.
   Номер строки умножается на ширину, а не на высоту. */
int image_offset(const Image *image, int x, int y);
int image_inside(const Image *image, int x, int y);
Pixel image_get(const Image *image, int x, int y);
void image_set(Image *image, int x, int y, Pixel color);
int image_pixel_count(const Image *image);

/* Тестовые изображения. Рисуют на изображении того размера, какой есть. */
void image_gradient(Image *image);
void image_checker(Image *image, int cell);
void image_stripes(Image *image, int count);

/* Фильтры над куском памяти: адрес первого пикселя и их количество. */
void pixels_invert(Pixel *data, int count);
void pixels_brightness(Pixel *data, int count, int delta);

/* То же, но проход приращением указателя до указателя на конец — адреса
   за последним элементом, который разыменовывать нельзя. */
void pixels_invert_walk(Pixel *begin, Pixel *end);

void image_invert(Image *image);
void image_brightness(Image *image, int delta);

/* Самая тёмная и самая светлая яркость: два результата через указатели,
   признак успеха — обычным значением. */
int image_min_max(const Image *image, int *out_min, int *out_max);

/* Обрезка и масштабирование ближайшим соседом: выделяется новый блок,
   старый освобождается. При неудаче изображение остаётся прежним. */
int image_crop(Image *image, int x, int y, int width, int height);
int image_resize(Image *image, int width, int height);

/* Поворот по часовой стрелке на угол, кратный 90 градусам; отрицательный
   угол — против часовой. Тоже выделяет новый блок: при повороте на 90
   ширина и высота меняются местами. Возвращает 1 при успехе. */
int image_rotate(Image *image, int angle);

/* Записывает изображение в текстовый формат PPM (P3).
   Возвращает 1 при успехе и 0, если файл не открылся. */
int image_save_ppm(const Image *image, const char *path);

#endif /* IMAGE_H */
