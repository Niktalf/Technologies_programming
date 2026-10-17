#ifndef SHELL_H
#define SHELL_H

#include "logfile.h"

/* Интерактивный цикл: команды count, head, tail, help, quit. */
void shell_run(LogFile *log);

#endif /* SHELL_H */
