#include <stdio.h>

#include "log_file.h"
#include "shell.h"

static LogFile log_file;

int main(int argc, char **argv)
{
    logfile_init(&log_file);

    if (argc >= 2) {
        const LogStatus status = logfile_load(&log_file, argv[1]);

        if (status != LOG_OK) {
            fprintf(stderr, "Error: %s (%s)\n", logfile_status_text(status), argv[1]);
            return 1;
        }
    }

    shell_run(&log_file);
    return 0;
}
