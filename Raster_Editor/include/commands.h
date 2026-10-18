#ifndef COMMANDS_H
#define COMMANDS_H

#include "editor.h"
#include "input.h"

/* Выполняет команду, заданную словами.
   Возвращает 0, если работу пора завершать, иначе 1. */
int commands_execute(Editor *editor, const Words *words);

void commands_print_help(void);

/* Проверка операций над цветом на краевых значениях и опыт с вычитанием
   в беззнаковом типе (практика 2). */
void commands_demo_colors(void);

#endif /* COMMANDS_H */
