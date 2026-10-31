#include <stdio.h>

#include "game.h"

/* ./dungeon             — случайный уровень, размер спросят при старте;
   ./dungeon level.txt   — уровень из текстового файла.
   Ошибки запуска уходят в stderr, и программа завершается с кодом 1. */
int main(int argc, char *argv[]) {
    if (argc > 2) {
        fprintf(stderr, "Usage: %s [level file]\n", argv[0]);
        return 1;
    }
    return game_run(argc == 2 ? argv[1] : NULL);
}
