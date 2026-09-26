#include <stdio.h>

#include "log_file.h"
#include "shell.h"

int main(int argc, char **argv)
{
    LogFile log;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <log file>\n", argv[0]);
        return 1;
    }

    logfile_init(&log);
    LogStatus status = logfile_scan(&log, argv[1]);
    if (status != LOG_OK) {
        fprintf(stderr, "Error: %s (%s)\n", logfile_status_text(status), argv[1]);
        return 1;
    }

    shell_run(&log);
    return 0;
}
