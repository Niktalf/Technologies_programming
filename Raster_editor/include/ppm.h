#ifndef PPM_H
#define PPM_H

#include "image.h"

typedef enum {
    PPM_OK = 0,
    PPM_ERR_OPEN,
    PPM_ERR_SIGNATURE,
    PPM_ERR_HEADER,
    PPM_ERR_SIZE,
    PPM_ERR_DATA,
} PpmStatus;

const char *ppm_status_text(PpmStatus status);
PpmStatus ppm_load(Image *image, const char *path);

#endif // PPM_H
