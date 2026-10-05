#ifndef SCRIPT_H
#define SCRIPT_H

#include "session.h"

int script_run(Session *session, const char *path, int stop_on_error);

#endif // SCRIPT_H
