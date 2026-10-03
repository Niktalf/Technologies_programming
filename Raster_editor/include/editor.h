#ifndef EDITOR_H
#define EDITOR_H

#include "image.h"

#define EDITOR_PATH_MAX 256

typedef struct {
    char  path[EDITOR_PATH_MAX];
    int   loaded;
    Image image;
} Editor;

void editor_init(Editor *editor);
void editor_set_path(Editor *editor, const char *path);
void editor_print_info(const Editor *editor);

#endif // EDITOR_H
