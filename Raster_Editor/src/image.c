#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "image.h"

int image_pixel_count(const Image *image) {
    if (image == NULL || image->pixels == NULL) {
        return 0;
    }
    return image->width * image->height;
}

ImgStatus image_create(Image *image, int width, int height) {
    if (image == NULL) {
        return IMG_EARG;
    }
    image->pixels = NULL;
    image->width = 0;
    image->height = 0;

    if (width <= 0 || height <= 0 || width > IMAGE_MAX_SIDE || height > IMAGE_MAX_SIDE) {
        return IMG_EARG;
    }
    /* Переполнение проверяется до умножения: делим предельное значение
       на высоту и сравниваем с шириной. */
    if ((size_t)width > ((size_t)-1 / sizeof(Pixel)) / (size_t)height) {
        return IMG_EARG;
    }
    Pixel *block = calloc((size_t)width * (size_t)height, sizeof(Pixel));
    if (block == NULL) {
        return IMG_ENOMEM;
    }
    image->pixels = block;
    image->width = width;
    image->height = height;
    return IMG_OK;
}

void image_destroy(Image *image) {
    if (image == NULL) {
        return;
    }
    free(image->pixels);
    image->pixels = NULL; /* повторное освобождение безопасно */
    image->width = 0;
    image->height = 0;
}

ImgStatus image_copy(Image *destination, const Image *source) {
    if (destination == NULL || source == NULL || source->pixels == NULL) {
        return IMG_EARG;
    }
    ImgStatus status = image_create(destination, source->width, source->height);
    if (status != IMG_OK) {
        return status;
    }
    memcpy(destination->pixels, source->pixels,
           (size_t)image_pixel_count(source) * sizeof(Pixel));
    return IMG_OK;
}

/* Подменяет изображение новым: старый блок освобождается только теперь,
   когда новый готов. Обратный порядок потерял бы данные при нехватке памяти. */
static void replace(Image *image, Image *fresh) {
    image_destroy(image);
    *image = *fresh;
}

ImgStatus image_crop(Image *image, int x, int y, int width, int height) {
    if (image == NULL || image->pixels == NULL) {
        return IMG_EARG;
    }
    /* Сравнение через вычитание: x + width могло бы переполнить int. */
    if (x < 0 || y < 0 || width <= 0 || height <= 0
        || x > image->width - width || y > image->height - height) {
        return IMG_EARG;
    }
    Image cropped;
    ImgStatus status = image_create(&cropped, width, height);
    if (status != IMG_OK) {
        return status;
    }
    for (int row = 0; row < height; ++row) {
        memcpy(&cropped.pixels[row * width],
               &image->pixels[(y + row) * image->width + x],
               (size_t)width * sizeof(Pixel));
    }
    replace(image, &cropped);
    return IMG_OK;
}

ImgStatus image_rotate(Image *image, int angle) {
    if (image == NULL || image->pixels == NULL || angle % 90 != 0) {
        return IMG_EARG;
    }
    /* Число четвертей оборота от 0 до 3: -90 — это то же, что 270. */
    int turns = ((angle / 90) % 4 + 4) % 4;
    if (turns == 0) {
        return IMG_OK;
    }
    Image rotated;
    ImgStatus status = image_create(&rotated,
                                    turns == 2 ? image->width : image->height,
                                    turns == 2 ? image->height : image->width);
    if (status != IMG_OK) {
        return status;
    }
    for (int y = 0; y < image->height; ++y) {
        for (int x = 0; x < image->width; ++x) {
            Pixel color = image_get(image, x, y);
            if (turns == 1) {
                /* 90 по часовой: столбец становится строкой */
                image_set(&rotated, image->height - 1 - y, x, color);
            } else if (turns == 2) {
                /* 180: отражение по обеим осям */
                image_set(&rotated, image->width - 1 - x, image->height - 1 - y, color);
            } else {
                /* 270 по часовой, то есть 90 против */
                image_set(&rotated, y, image->width - 1 - x, color);
            }
        }
    }
    replace(image, &rotated);
    return IMG_OK;
}

ImgStatus image_resize(Image *image, int width, int height) {
    if (image == NULL || image->pixels == NULL) {
        return IMG_EARG;
    }
    Image scaled;
    ImgStatus status = image_create(&scaled, width, height);
    if (status != IMG_OK) {
        return status;
    }
    /* Ближайший сосед: для каждой точки нового изображения берём ближайшую
       точку старого. Умножение до деления. */
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            scaled.pixels[y * width + x] =
                image_get(image, x * image->width / width, y * image->height / height);
        }
    }
    replace(image, &scaled);
    return IMG_OK;
}

int image_offset(const Image *image, int x, int y) {
    return y * image->width + x;
}

int image_inside(const Image *image, int x, int y) {
    return x >= 0 && y >= 0 && x < image->width && y < image->height;
}

Pixel image_get(const Image *image, int x, int y) {
    if (!image_inside(image, x, y)) {
        return pixel_pack(0, 0, 0);
    }
    return image->pixels[image_offset(image, x, y)];
}

void image_set(Image *image, int x, int y, Pixel color) {
    if (image_inside(image, x, y)) {
        image->pixels[image_offset(image, x, y)] = color;
    }
}

void image_gradient(Image *image) {
    /* Слева направо от чёрного к белому.
       Умножение перед делением: иначе все значения кроме последнего станут нулём. */
    for (int y = 0; y < image->height; ++y) {
        for (int x = 0; x < image->width; ++x) {
            int v = image->width > 1 ? x * CHANNEL_MAX / (image->width - 1) : 0;
            image_set(image, x, y, pixel_pack((uint8_t)v, (uint8_t)v, (uint8_t)v));
        }
    }
}

void image_checker(Image *image, int cell) {
    if (cell < 1) {
        cell = 1;
    }
    for (int y = 0; y < image->height; ++y) {
        for (int x = 0; x < image->width; ++x) {
            /* Номер клетки по x плюс номер по y: чётность суммы даёт шахматный узор.
               Если взять только одну из координат, получатся полосы. */
            int dark = ((x / cell) + (y / cell)) % 2;
            image_set(image, x, y, dark ? pixel_pack(40, 40, 40) : pixel_pack(220, 220, 220));
        }
    }
}

void image_stripes(Image *image, int count) {
    static const uint8_t PALETTE[][3] = {
        { 220, 40, 40 }, { 240, 160, 30 }, { 240, 230, 50 }, { 60, 190, 70 },
        { 40, 170, 220 }, { 60, 70, 200 }, { 150, 60, 190 }, { 230, 230, 230 }
    };
    const int palette_size = (int)(sizeof PALETTE / sizeof PALETTE[0]);

    if (count < 1) {
        count = 1;
    }
    for (int y = 0; y < image->height; ++y) {
        /* Номер полосы: умножение до деления, как в градиенте. */
        int band = y * count / image->height;
        const uint8_t *c = PALETTE[band % palette_size];
        for (int x = 0; x < image->width; ++x) {
            image_set(image, x, y, pixel_pack(c[0], c[1], c[2]));
        }
    }
}

void pixels_invert(Pixel *data, int count) {
    if (data == NULL) {
        return;
    }
    for (int i = 0; i < count; ++i) {
        data[i] = pixel_invert(data[i]);
    }
}

void pixels_brightness(Pixel *data, int count, int delta) {
    if (data == NULL) {
        return;
    }
    for (int i = 0; i < count; ++i) {
        data[i] = pixel_adjust_brightness(data[i], delta);
    }
}

void pixels_invert_walk(Pixel *begin, Pixel *end) {
    if (begin == NULL || end == NULL) {
        return;
    }
    /* Приращение указателя сдвигает адрес на размер элемента, а не на байт.
       Сравнение с end, а не с последним элементом: иначе потеряется
       ровно один пиксель. */
    for (Pixel *p = begin; p != end; ++p) {
        *p = pixel_invert(*p);
    }
}

void image_invert(Image *image) {
    /* В функцию уходит адрес блока пикселей, а не копия пикселей. */
    pixels_invert(image->pixels, image_pixel_count(image));
}

void image_brightness(Image *image, int delta) {
    pixels_brightness(image->pixels, image_pixel_count(image), delta);
}

ImgStatus image_min_max(const Image *image, int *out_min, int *out_max) {
    if (image == NULL || out_min == NULL || out_max == NULL) {
        return IMG_EARG;
    }
    int total = image_pixel_count(image);
    if (total <= 0) {
        return IMG_EARG; /* у пустого изображения нет ни минимума, ни максимума */
    }

    /* Начальные значения берём из первого пикселя, а не из нуля:
       иначе минимум всегда оказался бы нулём. */
    int low = pixel_luminance(image->pixels[0]);
    int high = low;
    for (int i = 1; i < total; ++i) {
        int value = pixel_luminance(image->pixels[i]);

        if (value < low) {
            low = value;
        }
        if (value > high) {
            high = value;
        }
    }
    *out_min = low;
    *out_max = high;
    return IMG_OK;
}
