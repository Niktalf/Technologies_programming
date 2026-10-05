#include "fill.h"

static Image     *target_image;
static Pixel      old_color;
static Pixel      new_color;
static FillReport current;

static void fill_step(const int x, const int y, const int depth) {
    if (!image_inside(target_image, x, y)) {
        return;
    }
    if (image_get(target_image, x, y) != old_color) {
        return;
    }
    if (depth > FILL_MAX_DEPTH) {
        current.truncated = 1;
        return;
    }

    image_set(target_image, x, y, new_color);
    current.filled++;
    if (depth > current.max_depth) {
        current.max_depth = depth;
    }

    fill_step(x + 1, y, depth + 1);
    fill_step(x - 1, y, depth + 1);
    fill_step(x, y + 1, depth + 1);
    fill_step(x, y - 1, depth + 1);
}

FillReport fill_region(Image *image, const int x, const int y, const Pixel color) {
    current.filled = 0;
    current.max_depth = 0;
    current.truncated = 0;

    if (!image_inside(image, x, y)) {
        return current;
    }

    target_image = image;
    old_color = image_get(image, x, y);
    new_color = color;

    if (old_color == new_color) {
        return current;
    }

    fill_step(x, y, 1);
    return current;
}
