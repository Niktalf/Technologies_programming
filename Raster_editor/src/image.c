#include <stdio.h>

#include "image.h"

void image_init(Image *image) {
    image->width = IMAGE_WIDTH;
    image->height = IMAGE_HEIGHT;
    for (int i = 0; i < IMAGE_WIDTH * IMAGE_HEIGHT; ++i) {
        image->pixels[i] = pixel_pack(0, 0, 0);
    }
}

int image_offset(const Image *image, const int x, const int y) {
    return y * image->width + x;
}

int image_inside(const Image *image, const int x, const int y) {
    return x >= 0 && y >= 0 && x < image->width && y < image->height;
}

Pixel image_get(const Image *image, const int x, const int y) {
    if (!image_inside(image, x, y)) {
        return pixel_pack(0, 0, 0);
    }
    return image->pixels[image_offset(image, x, y)];
}

void image_set(Image *image, const int x, const int y, const Pixel color) {
    if (image_inside(image, x, y)) {
        image->pixels[image_offset(image, x, y)] = color;
    }
}

void image_gradient(Image *image) {
    for (int y = 0; y < image->height; ++y) {
        for (int x = 0; x < image->width; ++x) {
            const int v = x * CHANNEL_MAX / (image->width - 1);
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
            const int dark = ((x / cell) + (y / cell)) % 2;
            image_set(image, x, y, dark ? pixel_pack(40, 40, 40) :
                pixel_pack(220, 220, 220));
        }
    }
}

void image_stripes(Image *image, int count) {
    static const uint8_t PALETTE[][3] = {
        { 220,  40,  40 }, { 240, 160,  30 }, { 240, 230,  50 },
        {  60, 190,  70 }, {  40, 170, 220 }, {  60,  70, 200 },
        { 150,  60, 190 }, { 230, 230, 230 }
    };
    const int palette_size = (int)(sizeof PALETTE / sizeof PALETTE[0]);

    if (count < 1) {
        count = 1;
    }
    for (int y = 0; y < image->height; ++y) {
        const int band = y * count / image->height;
        const uint8_t *c = PALETTE[band % palette_size];

        for (int x = 0; x < image->width; ++x) {
            image_set(image, x, y, pixel_pack(c[0], c[1], c[2]));
        }
    }
}

void pixels_invert(Pixel *data, const int count) {
    if (data == NULL) {
        return;
    }
    for (int i = 0; i < count; ++i) {
        data[i] = pixel_invert(data[i]);
    }
}

void pixels_brightness(Pixel *data, const int count, const int delta) {
    if (data == NULL) {
        return;
    }
    for (int i = 0; i < count; ++i) {
        data[i] = pixel_adjust_brightness(data[i], delta);
    }
}

void pixels_invert_walk(Pixel *begin, const Pixel *end) {
    if (begin == NULL || end == NULL) {
        return;
    }
    for (Pixel *p = begin; p != end; ++p) {
        *p = pixel_invert(*p);
    }
}

void image_invert(Image *image) {
    pixels_invert(image->pixels, image->width * image->height);
}

void image_brightness(Image *image, const int delta) {
    pixels_brightness(image->pixels, image->width * image->height, delta);
}

int image_min_max(const Image *image, int *out_min, int *out_max) {
    if (image == NULL || out_min == NULL || out_max == NULL) {
        return 0;
    }
    const int total = image->width * image->height;
    if (total <= 0) {
        return 0;
    }

    int low = pixel_luminance(image->pixels[0]);
    int high = low;
    for (int i = 1; i < total; ++i) {
        const int value = pixel_luminance(image->pixels[i]);

        if (value < low) {
            low = value;
        }
        if (value > high) {
            high = value;
        }
    }
    *out_min = low;
    *out_max = high;
    return 1;
}

int image_save_ppm(const Image *image, const char *path)
{
    FILE *file = fopen(path, "w");

    if (file == NULL) {
        return 0;
    }
    fprintf(file, "P3\n%d %d\n%d\n", image->width, image->height, CHANNEL_MAX);

    for (int y = 0; y < image->height; ++y) {
        for (int x = 0; x < image->width; ++x) {
            Pixel p = image_get(image, x, y);
            fprintf(file, "%u %u %u ",
                    (unsigned)pixel_red(p), (unsigned)pixel_green(p), (unsigned)pixel_blue(p));
        }
        fputc('\n', file);
    }
    fclose(file);
    return 1;
}
