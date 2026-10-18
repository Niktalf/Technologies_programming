#include <stdio.h>

#include "logfile.h"
#include "shell.h"

/* Журнал весит около трёх мегабайт и поэтому статический:
   в стеке функции main он бы не поместился. */
static LogFile log_file;

int main(int argc, char *argv[]) {
    logfile_init(&log_file);

    /* Файл можно указать сразу, а можно загрузить потом командой load.
       Ошибки запуска идут в поток ошибок, ответы на команды — в обычный вывод. */
    if (argc >= 2) {
        LogStatus status = logfile_load(&log_file, argv[1]);
        if (status != LOG_OK) {
            fprintf(stderr, "Error: %s (%s)\n", logfile_status_text(status), argv[1]);
            return 1;
        }
    }

    shell_run(&log_file);
    return 0;
}
