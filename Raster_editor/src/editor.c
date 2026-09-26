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
}

void editor_set_path(Editor *editor, const char *path)
{
    if (editor == NULL || path == NULL) {
        return;
    }
    strncpy(editor->path, path, sizeof editor->path - 1);
    editor->path[sizeof editor->path - 1] = '\0';
    editor->loaded = 0;
}

void editor_print_info(const Editor *editor)
{
    if (editor == NULL) {
        return;
    }
    if (editor->path[0] == '\0') {
        printf("The file is not specified.\n");
        return;
    }
    printf("File: %s\n", editor->path);
    printf("Status: %s\n", editor->loaded ? "loaded" : "not loaded");
    printf("Size: unknown\n");
}
