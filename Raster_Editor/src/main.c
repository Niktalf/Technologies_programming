#include <stdio.h>

#include "commands.h"
#include "editor.h"
#include "input.h"

/* Буфер под строку команды: 126 символов, перевод строки и завершающий ноль. */
#define LINE_BUFFER_SIZE 128

/* Ошибки запуска, после которых программа завершается с ненулевым кодом,
   идут в поток ошибок: так их видно, даже если обычный вывод перенаправлен
   в файл. Ответы на команды — в обычный вывод. */
static void print_usage(const char *program_name) {
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "  %s <file> <operation>  run one operation\n", program_name);
    fprintf(stderr, "  %s                    interactive mode\n", program_name);
}

/* Печатает число аргументов и каждый из них. Нулевой аргумент — имя самой
   программы, полезные начинаются с первого. */
static void print_arguments(int argc, char *argv[]) {
    printf("Arguments: %d\n", argc);
    for (int i = 0; i < argc; ++i) {
        printf("  argv[%d] = %s\n", i, argv[i]);
    }
}

/* Режим одной операции: «программа файл операция».
   Число аргументов проверяется до обращения к ним: argv[2] при argc == 2
   — это уже не строка, а нулевой указатель. */
static int run_once(int argc, char *argv[]) {
    print_arguments(argc, argv);
    if (argc != 3) {
        print_usage(argv[0]);
        return 1;
    }

    const char *program = argv[0];
    const char *file = argv[1];
    const char *operation_word = argv[2];
    printf("Program:   %s\n", program);
    printf("File:      %s\n", file);
    printf("Operation: %s\n", operation_word);

    Operation operation = operation_from_word(operation_word);
    if (operation == OP_UNKNOWN) {
        fprintf(stderr, "Unknown operation: %s\n", operation_word);
        return 1;
    }

    Editor editor;
    editor_init(&editor);
    editor_set_path(&editor, file);
    operation_execute(operation, &editor, operation_word);
    return 0;
}

/* Интерактивный режим: операции по одной, пока не будет команды выхода
   или ввод не кончится. Обе причины сводятся к одному флагу running —
   у цикла одно очевидное условие выхода. */
static int run_interactive(void) {
    Editor editor;
    editor_init(&editor);
    printf("Raster editor. Type help for the list of operations.\n");

    char line[LINE_BUFFER_SIZE];
    int running = 1;
    while (running) {
        printf("> ");
        fflush(stdout);

        if (input_read_line(line, sizeof line)) {
            Operation operation = operation_from_word(line);
            running = operation_execute(operation, &editor, line);
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
