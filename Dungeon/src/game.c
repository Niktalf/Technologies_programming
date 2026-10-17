#include <stdio.h>
#include <string.h>

#include "game.h"

/* Буфер под команду: 15 символов и завершающий ноль. Число 15 повторяется
   в формате scanf("%15s"): ограничение длины не даёт длинному вводу
   записать что-то за концом буфера. */
#define COMMAND_SIZE 16

/* Практика 1: карта — просто картинка из строковых литералов.
   Никакой структуры данных за ней пока нет: герой нарисован прямо
   в строке и никуда не сдвигается. */
static void print_picture(void) {
    printf("##########################\n");
    printf("#@.......#.......$.......#\n");
    printf("#........#...............#\n");
    printf("#....$...#.......#########\n");
    printf("#................#.......#\n");
    printf("#######..........#...>...#\n");
    printf("#........................#\n");
    printf("##########################\n");
}

static void print_commands(void) {
    printf("Commands: north, south, east, west, look, quit\n");
}

void game_run(void) {
    /* Счётчик объявлен до цикла. Переменная, объявленная внутри тела
       цикла, создавалась бы заново на каждом проходе и всегда была бы
       равна начальному значению. */
    int turns = 0;
    int running = 1;

    printf("Welcome to the dungeon!\n\n");
    print_picture();
    printf("\n");
    print_commands();

    /* Цикл, а в нём ветвление и последовательность действий: все три
       конструкции структурного программирования. Выход один — через
       условие цикла, без операторов безусловного перехода. */
    while (running) {
        printf("\nTurn %d > ", turns);
        fflush(stdout);

        char command[COMMAND_SIZE];
        if (scanf("%15s", command) != 1) {
            /* Ввод закончился (Ctrl+D или конец файла). Команды quit уже
               не будет, и без этой ветки цикл никогда бы не остановился. */
            running = 0;
        } else if (strcmp(command, "quit") == 0) {
            /* quit проверяется в той же цепочке, что и остальные команды,
               поэтому она никогда не попадёт в ветку «неизвестная команда». */
            running = 0;
        } else if (strcmp(command, "north") == 0) {
            printf("The hero goes north.\n");
            ++turns;
        } else if (strcmp(command, "south") == 0) {
            printf("The hero goes south.\n");
            ++turns;
        } else if (strcmp(command, "east") == 0) {
            printf("The hero goes east.\n");
            ++turns;
        } else if (strcmp(command, "west") == 0) {
            printf("The hero goes west.\n");
            ++turns;
        } else if (strcmp(command, "look") == 0) {
            /* Осмотреться — не шаг, поэтому ход не тратится. */
            print_picture();
        } else {
            printf("Unknown command: %s\n", command);
            print_commands();
        }
    }

    printf("\nGame over. Turns made: %d\n", turns);
}
