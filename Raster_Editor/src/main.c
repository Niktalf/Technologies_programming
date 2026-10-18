#include <stdio.h>

#include "commands.h"
#include "editor.h"
#include "input.h"

/* Редактор весит больше 256 КБ из-за изображения внутри,
   поэтому он статический: в стеке функции он бы не поместился. */
static Editor editor;

/* Ошибки запуска, после которых программа завершается с ненулевым кодом,
   идут в поток ошибок: так их видно, даже если обычный вывод перенаправлен
   в файл. Ответы на команды — в обычный вывод. */
static void print_usage(const char *program_name) {
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "  %s                            interactive mode\n", program_name);
    fprintf(stderr, "  %s <file> <operation> [args]  one operation, result to the file\n",
            program_name);
    fprintf(stderr, "Example:\n");
    fprintf(stderr, "  %s checker.ppm gen checker 16\n", program_name);
}

/* Печатает число аргументов и каждый из них. Нулевой аргумент — имя самой
   программы, полезные начинаются с первого. */
static void print_arguments(int argc, char *argv[]) {
    printf("Arguments: %d\n", argc);
    for (int i = 0; i < argc; ++i) {
        printf("  argv[%d] = %s\n", i, argv[i]);
    }
}

/* Одна операция. Если после неё есть изображение, оно записывается в файл:
   так редактор можно вызывать из скриптов для генерации тестовых картинок.
   Число аргументов проверяется до обращения к ним. */
static int run_once(int argc, char *argv[]) {
    print_arguments(argc, argv);
    if (argc < 3) {
        print_usage(argv[0]);
        return 1;
    }
    editor_init(&editor);
    editor_set_path(&editor, argv[1]);

    Words words;
    input_from_args(&words, argc - 2, argv + 2);
    commands_execute(&editor, &words);

    if (!editor.loaded) {
        return 0;
    }
    if (!image_save_ppm(&editor.image, editor.path)) {
        fprintf(stderr, "Cannot write the file: %s\n", editor.path);
        return 1;
    }
    printf("Written: %s\n", editor.path);
    return 0;
}

/* Интерактивный режим: операции по одной, пока не будет команды выхода
   или ввод не кончится. Обе причины сводятся к одному флагу running. */
static int run_interactive(void) {
    editor_init(&editor);
    printf("Raster editor. Type help for the list of operations.\n");

    int running = 1;
    while (running) {
        printf("> ");
        fflush(stdout);

        Words words;
        if (input_read_words(&words)) {
            running = commands_execute(&editor, &words);
        } else {
            running = 0;
        }
    }
    printf("Done.\n");
    return 0;
}

int main(int argc, char *argv[]) {
    /* Режим зависит только от числа аргументов. */
    if (argc > 1) {
        printf("Mode: one operation from the command line\n");
        return run_once(argc, argv);
    }
    printf("Mode: interactive\n");
    return run_interactive();
}
