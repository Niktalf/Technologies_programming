#include <stdio.h>
#include <string.h>

#include "cli.h"
#include "parser.h"
#include "script.h"

#define SCRIPT_LINE_MAX 512

int script_run(Session *session, const char *path, const int stop_on_error) {
    FILE *file = fopen(path, "r");
    char line[SCRIPT_LINE_MAX];
    char *argv[PARSER_ARGS_MAX];
    long number = 0;
    long executed = 0;
    long failed = 0;

    if (file == NULL) {
        printf("Couldn't open script: %s\n", path);
        return 0;
    }

    while (fgets(line, (int)sizeof line, file) != NULL) {
         ++number;

        if (strchr(line, '\n') == NULL && !feof(file)) {
            int c;
            while ((c = fgetc(file)) != EOF && c != '\n') {}
            printf("Line %ld: too long, cut off\n", number);
        }

        char *p = line;
        while (*p == ' ' || *p == '\t') {
            ++p;
        }
        if (*p == '\0' || *p == '\n' || *p == '#') {
            continue;
        }

        int count = 0;
        const ParserStatus status = parser_split(p, argv, PARSER_ARGS_MAX, &count);
        if (status != PARSER_OK) {
            printf("Line %ld: %s\n", number, parser_status_text(status));
            ++failed;
            if (stop_on_error) {
                printf("The execution was interrupted.\n");
                break;
            }
            continue;
        }
        if (count == 0) {
            continue;
        }

        ++executed;
        if (cli_execute(session, count, argv) == 0) {
            printf("Line %ld: exit command, script stopped\n", number);
            break;
        }
    }

    fclose(file);
    printf("Lines in the file: %ld, commands executed: %ld, parsing errors: %ld\n", number, executed, failed);
    return failed == 0;
}
