#include <stdio.h>

#include "logfile.h"
#include "shell.h"

int main(int argc, char *argv[]) {
    /* Сам объект журнала маленький: всё содержимое лежит в куче. */
    LogFile log_file;
    logfile_init(&log_file);

    /* Файл можно указать сразу, а можно загрузить потом командой load.
       Ошибки запуска идут в поток ошибок, ответы на команды — в обычный вывод. */
    if (argc >= 2) {
        LogStatus status = logfile_load(&log_file, argv[1]);
        if (status != LOG_OK) {
            fprintf(stderr, "Error: %s (%s)\n", status_text(status), argv[1]);
            logfile_destroy(&log_file);
            return 1;
        }
    }

    shell_run(&log_file);
    logfile_destroy(&log_file);
    return 0;
}
