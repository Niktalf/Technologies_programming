#include <stdio.h>
#include <string.h>

#include "editor.h"

void editor_init(Editor *editor)
{
    if (editor == NULL) {
        return;
    }
    editor->path[0] = '\0';
    editor->loaded = 0;
    image_init(&editor->image);
}

void editor_set_path(Editor *editor, const char *path)
{
    if (editor == NULL || path == NULL) {
        return;
    }
    strncpy(editor->path, path, sizeof editor->path - 1);
    editor->path[sizeof editor->path - 1] = '\0';
}

void editor_print_info(const Editor *editor)
{
    if (editor == NULL) {
        return;
    }
    printf("File: %s\n", editor->path[0] != '\0' ? editor->path : "not specified");
    if (!editor->loaded) {
        printf("There is no image. Create it using the command gen.\n");
        return;
    }
    printf("Size: %d x %d\n", editor->image.width, editor->image.height);
    printf("Upper-left pixel: 0x%06X\n", (unsigned)image_get(&editor->image, 0, 0));
    printf("Lower right pixel: 0x%06X\n",
           (unsigned)image_get(&editor->image, editor->image.width - 1, editor->image.height - 1));
}
