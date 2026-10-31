#ifndef EDITOR_H
#define EDITOR_H

#include "image.h"

/* Состояние редактора: имя файла и само изображение. */

#define EDITOR_PATH_MAX 256

typedef struct {
    char path[EDITOR_PATH_MAX]; /* пустая строка означает, что файл не задан */
    int loaded;                /* есть ли в редакторе изображение */
    Image image;               /* пиксели в куче; image.pixels == NULL — изображения нет */
} Editor;

/* Изображение теперь в куче, поэтому сам редактор занимает несколько
   сотен байт и спокойно помещается в стек. */

void editor_init(Editor *editor);

/* Освобождает изображение. Вызывается перед выходом из функции,
   где объявлен редактор: каждому выделению — ровно одно освобождение. */
void editor_destroy(Editor *editor);
void editor_set_path(Editor *editor, const char *path);
void editor_print_info(const Editor *editor);

#endif /* EDITOR_H */
