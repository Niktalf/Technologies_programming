#ifndef LEVEL_H
#define LEVEL_H

#include <stdint.h>

/* Практика 2: уровни важности и фильтр по ним.
   Фильтр хранится одной переменной: каждому уровню соответствует свой бит. */

typedef enum {
    LEVEL_DEBUG = 0,
    LEVEL_INFO,
    LEVEL_WARN,
    LEVEL_ERROR,
    LEVEL_FATAL,
    LEVEL_COUNT,
    /* Отдельное значение для мусора: без него строка с опечаткой
       молча превратится в отладочное сообщение. */
    LEVEL_UNKNOWN
} Level;

typedef uint8_t LevelMask;

#define LEVEL_BIT(level) ((LevelMask)(1u << (level)))
#define LEVEL_MASK_ALL   ((LevelMask)((1u << LEVEL_COUNT) - 1u))
#define LEVEL_MASK_NONE  ((LevelMask)0u)

/* Перевод слова в уровень и обратно. Регистр значения не имеет. */
Level level_from_word(const char *word);
const char *level_name(Level level);

/* Операции над набором уровней. */
LevelMask level_mask_set(LevelMask mask, Level level);
LevelMask level_mask_clear(LevelMask mask, Level level);
int level_mask_has(LevelMask mask, Level level);

/* Печатает список уровней, входящих в маску. */
void level_mask_print(LevelMask mask);

#endif /* LEVEL_H */
