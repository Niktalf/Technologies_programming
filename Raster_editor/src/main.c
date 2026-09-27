#include <stdio.h>

#include "commands.h"
#include "editor.h"
#include "input.h"

#define LINE_BUFFER_SIZE 128

static void print_usage(const char *program_name)
{
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "  %s <file> <operation>    perform one operation\n", program_name);
    fprintf(stderr, "  %s                       interactive mode\n", program_name);
}

static int run_batch(const int argc, char **argv)
{
    Editor editor;

    if (argc < 3) {
        print_usage(argv[0]);
        return 1;
    }
    editor_init(&editor);
    editor_set_path(&editor, argv[1]);

    const Operation operation = operation_from_word(argv[2]);
    if (operation == OP_UNKNOWN) {
        fprintf(stderr, "Unknown operation: %s\n", argv[2]);
        return 1;
    }
    operation_execute(operation, &editor, argv[2]);
    return 0;
}

static int run_interactive()
{
    Editor editor;
    char line[LINE_BUFFER_SIZE];
    int running = 1;

    editor_init(&editor);
    printf("Raster editor. Type help for a list of operations.\n");

    while (running) {
        printf("> ");
        fflush(stdout);

        if (!input_read_line(line, sizeof line)) {
            break;
        }
        const Operation operation = operation_from_word(line);
        running = operation_execute(operation, &editor, line);
    }
    printf("The work is completed.\n");
    return 0;
}

int main(int argc, char **argv)
{
    if (argc > 1) {
        return run_batch(argc, argv);
    }
    return run_interactive();
}
