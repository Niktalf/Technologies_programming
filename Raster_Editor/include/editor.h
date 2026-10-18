#ifndef EDITOR_H
#define EDITOR_H

#include "image.h"

/* Состояние редактора: имя файла и само изображение. */

#define EDITOR_PATH_MAX 256

typedef struct {
    char path[EDITOR_PATH_MAX]; /* пустая строка означает, что файл не задан */
    int loaded;                /* есть ли в редакторе изображение */
    Image image;
    Image scratch;             /* второй массив для поворота */
} Editor;

/* Editor содержит два изображения и весит больше 512 КБ,
   поэтому объект редактора объявляется статическим, а не в стеке. */

void editor_init(Editor *editor);
void editor_set_path(Editor *editor, const char *path);
void editor_print_info(const Editor *editor);

#endif /* EDITOR_H */
