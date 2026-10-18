#include <stdio.h>
#include <string.h>

#include "cli.h"
#include "parser.h"
#include "script.h"

#define SCRIPT_LINE_MAX 512

/* Итоги выполнения скрипта. */
typedef struct {
    long lines;    /* прочитано строк */
    long executed; /* выполнено команд */
    long failed;   /* строк с ошибками */
} ScriptReport;

/* Дочитывает и выбрасывает остаток строки до перевода строки или конца файла. */
static void skip_rest_of_line(FILE *file) {
    int c = fgetc(file);
    while (c != '\n' && c != EOF) {
        c = fgetc(file);
    }
}

/* Пустая строка или комментарий: после пробелов ничего нет или стоит решётка. */
static int is_blank_or_comment(const char *p) {
    while (*p == ' ' || *p == '\t') {
        ++p;
    }
    return *p == '\0' || *p == '\n' || *p == '\r' || *p == '#';
}

/* Выполняет одну строку скрипта. Возвращает CLI_QUIT, CLI_OK или CLI_ERROR;
   о любой ошибке сообщает с номером строки. */
static int run_line(Session *session, char *line, long number, ScriptReport *report) {
    if (is_blank_or_comment(line)) {
        return CLI_OK;
    }
    char *argv[PARSER_ARGS_MAX];
    int count = 0;
    ParserStatus status = parser_split(line, argv, PARSER_ARGS_MAX, &count);
    if (status != PARSER_OK) {
        printf("Line %ld: %s\n", number, parser_status_text(status));
        return CLI_ERROR;
    }
    if (count == 0) {
        return CLI_OK;
    }
    ++report->executed;
    int result = cli_execute(session, count, argv);
    if (result == CLI_QUIT) {
        printf("Line %ld: exit command, script stopped\n", number);
    } else if (result == CLI_ERROR) {
        printf("Line %ld: command failed\n", number);
    }
    return result;
}

int script_run(Session *session, const char *path, int stop_on_error) {
    FILE *file = fopen(path, "r");
    if (file == NULL) {
        printf("Cannot open the script: %s\n", path);
        return 0;
    }

    ScriptReport report = { 0, 0, 0 };
    /* Скрипт прерывают команда выхода и, если просили, первая ошибка.
       Обе причины сводятся к одному флагу. */
    int running = 1;
    char line[SCRIPT_LINE_MAX];
    while (running && fgets(line, (int)sizeof line, file) != NULL) {
        ++report.lines;
        /* Хвост слишком длинной строки дочитываем, иначе он станет
           отдельной командой с другим номером. */
        if (strchr(line, '\n') == NULL && !feof(file)) {
            skip_rest_of_line(file);
            printf("Line %ld: too long, truncated\n", report.lines);
        }
        int result = run_line(session, line, report.lines, &report);
        if (result == CLI_ERROR) {
            ++report.failed;
            if (stop_on_error) {
                printf("Execution stopped.\n");
                running = 0;
            }
        } else if (result == CLI_QUIT) {
            running = 0;
        }
    }

    fclose(file);
    printf("Lines in the file: %ld, commands run: %ld, errors: %ld\n",
           report.lines, report.executed, report.failed);
    return report.failed == 0;
}
