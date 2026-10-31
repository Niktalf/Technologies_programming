#ifndef GAME_H
#define GAME_H

#include "entities.h"
#include "map.h"

/* Практика 8: всё состояние игры — одна структура. Функции получают её
   адрес, а не полтора десятка отдельных параметров, как на практике 7. */
typedef struct {
    Map map;
    MonsterList monsters;
    Hero hero;
    int turns;
} Game;

/* Запускает игру. Если level_path не NULL, уровень загружается из этого
   файла, иначе игра спрашивает размер и строит случайный уровень.
   Возвращает код завершения программы: 1, если файл уровня не загрузился. */
int game_run(const char *level_path);

#endif /* GAME_H */
