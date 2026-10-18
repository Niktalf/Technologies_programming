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

/* Фильтры над куском памяти. Принимают адрес первого пикселя и их количество,
   а не копию изображения: копия весит четверть мегабайта, и менять её
   бессмысленно — изменения не вернутся к вызывающему. */
void pixels_invert(Pixel *data, int count);
void pixels_brightness(Pixel *data, int count, int delta);

/* То же самое, но проход идёт не по индексам, а приращением указателя:
   указатель на конец — это адрес за последним элементом, и разыменовывать
   его нельзя, он нужен только для сравнения. */
void pixels_invert_walk(Pixel *begin, Pixel *end);

/* Записывает изображение в текстовый формат PPM (P3).
   Возвращает 1 при успехе и 0, если файл не открылся. */
int image_save_ppm(const Image *image, const char *path);

/* Практика 6: обрезка и поворот. Массив фиксированный, поэтому меняются
   только ширина и высота, а пиксели переезжают внутри того же массива.
   Обе функции возвращают 1 при успехе и 0 при неверных аргументах. */
int image_crop(Image *image, int x, int y, int width, int height);

/* Поворот по часовой стрелке на угол, кратный 90 градусам; отрицательный
   угол — против часовой. Для поворота на 90 и 270 нужен второй массив:
   пиксели не переставить на месте, не затерев те, что ещё не скопированы.
   Его даёт вызывающий через scratch — своего места под второе изображение
   у функции нет, а в стек четверть мегабайта класть нельзя. */
int image_rotate(Image *image, int angle, Image *scratch);

/* Самая тёмная и самая светлая яркость сразу: два результата возвращаются
   через указатели, а признак успеха — обычным значением. */
int image_min_max(const Image *image, int *out_min, int *out_max);

#endif /* IMAGE_H */
