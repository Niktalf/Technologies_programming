#ifndef IMAGE_H
#define IMAGE_H

#include "pixel.h"

#define IMAGE_WIDTH  256
#define IMAGE_HEIGHT 256

typedef struct {
    Pixel pixels[IMAGE_WIDTH * IMAGE_HEIGHT];
    int   width;
    int   height;
} Image;

void image_init(Image *image);
int image_offset(const Image *image, int x, int y);

int   image_inside(const Image *image, int x, int y);
Pixel image_get(const Image *image, int x, int y);
void  image_set(Image *image, int x, int y, Pixel color);

void image_gradient(Image *image);
void image_checker(Image *image, int cell);
void image_stripes(Image *image, int count);

void pixels_invert(Pixel *data, int count);
void pixels_brightness(Pixel *data, int count, int delta);
void pixels_invert_walk(Pixel *begin, const Pixel *end);

void image_invert(Image *image);
void image_brightness(Image *image, int delta);

int image_min_max(const Image *image, int *out_min, int *out_max);
int image_save_ppm(const Image *image, const char *path);

#endif // IMAGE_H
