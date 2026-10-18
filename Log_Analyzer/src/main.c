#include <stdio.h>

#include "logfile.h"
#include "shell.h"

int main(int argc, char *argv[]) {
    /* Число аргументов проверяется до обращения к argv[1]. Ошибки запуска
       идут в поток ошибок, ответы на команды — в обычный вывод. */
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <log file>\n", argv[0]);
        return 1;
    }

    LogFile log;
    LogStatus status = logfile_scan(&log, argv[1], LEVEL_MASK_ALL);
    if (status != LOG_OK) {
        fprintf(stderr, "Error: %s (%s)\n", logfile_status_text(status), argv[1]);
        return 1;
    }

    shell_run(&log);
    return 0;
}
