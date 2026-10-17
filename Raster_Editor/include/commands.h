#ifndef COMMANDS_H
#define COMMANDS_H

#include "editor.h"

/* Операции редактора. OP_EMPTY — пустая строка: ничего не делать. */
typedef enum {
    OP_UNKNOWN = 0,
    OP_INFO,
    OP_HELP,
    OP_QUIT,
    OP_EMPTY
} Operation;

/* Распознаёт имя операции. Слово уже приведено к нижнему регистру. */
Operation operation_from_word(const char *word);

/* Выполняет операцию. Возвращает 0, если работу пора завершать, иначе 1. */
int operation_execute(Operation operation, Editor *editor, const char *word);

void commands_print_help(void);

#endif /* COMMANDS_H */
