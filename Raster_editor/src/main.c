#include <stdio.h>

#include "commands.h"
#include "editor.h"
#include "input.h"

static Editor editor;

static void print_usage(const char *program_name)
{
    printf("Usage:\n");
    printf("  %s                               interactive mode\n", program_name);
    printf("  %s <file> <operation> [arguments] one operation, result in file\n", program_name);
    printf("For example:\n");
    printf("  %s checker.ppm gen checker 16\n", program_name);
}

static int run_once(const int argc, char **argv)
{
    if (argc < 3) {
        print_usage(argv[0]);
        return 1;
    }
    editor_init(&editor);
    editor_set_path(&editor, argv[1]);

    Words words;
    input_from_args(&words, argc - 2, argv + 2);
    commands_execute(&editor, &words);

    if (editor.loaded) {
        if (!image_save_ppm(&editor.image, editor.path)) {
            printf("No need to write a file: %s\n", editor.path);
            return 1;
        }
        printf("Recorded: %s\n", editor.path);
    }
    return 0;
}

static void run_interactive()
{
    Words words;

    editor_init(&editor);
    printf("Bitmap editor. Type 'help' for a list of operations.\n");

    while (1) {
        printf("> ");
        fflush(stdout);

        if (!input_read_words(&words)) {
            break;
        }
        if (!commands_execute(&editor, &words)) {
            break;
        }
    }
    printf("The work is completed.\n");
}

int main(int argc, char **argv)
{
    if (argc > 1) {
        return run_once(argc, argv);
    }
    run_interactive();
    return 0;
}
