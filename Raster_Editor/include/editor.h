#ifndef EDITOR_H
#define EDITOR_H

/* Состояние редактора.
   На практике 3 здесь появится само изображение, пока хранится только имя файла. */

#define EDITOR_PATH_MAX 256

typedef struct {
    char path[EDITOR_PATH_MAX]; /* пустая строка означает, что файл не задан */
    int loaded;                 /* признак загруженного изображения */
} Editor;

void editor_init(Editor *editor);

/* Запоминает имя файла. Само чтение появится на практике 6. */
void editor_set_path(Editor *editor, const char *path);

/* Печатает сведения о текущем изображении. */
void editor_print_info(const Editor *editor);

#endif /* EDITOR_H */
