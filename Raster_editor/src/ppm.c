#include <ctype.h>
#include <stdio.h>

#include "ppm.h"

const char *ppm_status_text(const PpmStatus status) {
    switch (status) {
        case PPM_OK:            return "success";
        case PPM_ERR_OPEN:      return "file did not open";
        case PPM_ERR_SIGNATURE: return "this is not a P3 file";
        case PPM_ERR_HEADER:    return "corrupted header";
        case PPM_ERR_SIZE:      return "image is too big";
        case PPM_ERR_DATA:      return "the numbers ran out earlier than the pixels";
        default:                return "unknown error";
    }
}

static void skip_spaces_and_comments(FILE *file) {
    int c = fgetc(file);

    while (c != EOF) {
        if (c == '#') {
            while (c != EOF && c != '\n') {
                c = fgetc(file);
            }
        } else if (!isspace(c)) {
            ungetc(c, file);
            return;
        }
        c = fgetc(file);
    }
}

static int read_number(FILE *file, int *out) {
    skip_spaces_and_comments(file);
    return fscanf(file, "%d", out) == 1;
}

static Image scratch;

PpmStatus ppm_load(Image *image, const char *path)
{
    FILE *file = fopen(path, "r");
    if (file == NULL) {
        return PPM_ERR_OPEN;
    }

    skip_spaces_and_comments(file);
    char signature[3];
    if (fscanf(file, "%2s", signature) != 1 || signature[0] != 'P' || signature[1] != '3') {
        fclose(file);
        return PPM_ERR_SIGNATURE;
    }

    int width, height, maximum;
    if (!read_number(file, &width) || !read_number(file, &height)
        || !read_number(file, &maximum)) {
        fclose(file);
        return PPM_ERR_HEADER;
    }
    if (width <= 0 || height <= 0 || maximum <= 0 || maximum > 65535) {
        fclose(file);
        return PPM_ERR_HEADER;
    }
    if (width > IMAGE_WIDTH || height > IMAGE_HEIGHT) {
        fclose(file);
        return PPM_ERR_SIZE;
    }

    image_init(&scratch);
    scratch.width = width;
    scratch.height = height;

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int red;
            int green;
            int blue;

            if (!read_number(file, &red) || !read_number(file, &green)
                || !read_number(file, &blue)) {
                fclose(file);
                return PPM_ERR_DATA;
            }
            if (maximum != CHANNEL_MAX) {
                red = red * CHANNEL_MAX / maximum;
                green = green * CHANNEL_MAX / maximum;
                blue = blue * CHANNEL_MAX / maximum;
            }
            image_set(&scratch, x, y, pixel_pack(
                (uint8_t)channel_clamp(red),
                (uint8_t)channel_clamp(green),
                (uint8_t)channel_clamp(blue)
            ));
        }
    }

    fclose(file);
    *image = scratch;
    return PPM_OK;
}
