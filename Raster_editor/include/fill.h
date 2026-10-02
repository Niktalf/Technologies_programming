#ifndef FILL_H
#define FILL_H

#include "image.h"

#define FILL_MAX_DEPTH 10000

typedef struct {
    int filled;
    int max_depth;
    int truncated;
} FillReport;

FillReport fill_region(Image *image, int x, int y, Pixel color);

#endif // FILL_H
