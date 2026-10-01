#ifndef COMMANDS_H
#define COMMANDS_H

#include "editor.h"
#include "input.h"

int commands_execute(Editor *editor, const Words *words);

void commands_print_help();

void commands_demo_colors();

#endif // COMMANDS_H
