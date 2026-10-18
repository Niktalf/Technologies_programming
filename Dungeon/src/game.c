#include <stdio.h>
#include <string.h>

#include "game.h"
#include "hero.h"
#include "map.h"
#include "reach.h"

/* Практика 5: глобальных переменных больше нет. Карта и характеристики
   героя — локальные переменные game_run, и функции получают их адреса.
   По списку параметров видно, что функция может изменить: всё, что
   передано указателем без const, и только это. */

#define COMMAND_SIZE 16

/* Счётчик вызовов команды addr. Переменная вне функций живёт всё время
   работы программы. Нужна, чтобы показать, где лежат такие переменные. */
static int addr_calls = 0;

/* Названия состояний в порядке битов: элемент номер i описывает бит номер i. */
static const char *const STATE_NAMES[STATE_COUNT] = {
    "poisoned", "blinded", "invisible", "petrified", "burning", "asleep"
};

static void print_commands(void) {
    printf("Commands: north, south, east, west - step; look - map;\n");
    printf("         check - is the exit reachable; cell - what is underfoot;\n");
    printf("         addr - addresses in memory; status, hit <N>,\n");
    printf("         on/off/toggle <state>, stacktest <N>, quit\n");
}

static void skip_rest_of_line(void) {
    int c = getchar();
    while (c != '\n' && c != EOF) {
        c = getchar();
    }
}

/* Получает значения, а не адреса: функция только читает. */
static void print_status(int hp, int hp_max, unsigned char attack, unsigned int gold,
                         int depth, unsigned int states) {
    printf("Health: %d / %d (%.1f%%)\n", hp, hp_max, (double)hp / (double)hp_max * 100.0);
    printf("Attack: %u, gold: %u, floor: %d\n", (unsigned)attack, gold, depth);
    printf("States:");
    int any = 0;
    for (int i = 0; i < STATE_COUNT; ++i) {
        if ((states & (1u << i)) != 0) {
            printf(" %s", STATE_NAMES[i]);
            any = 1;
        }
    }
    printf("%s\n", any ? "" : " none");
}

static int direction_number(const char *word) {
    if (strcmp(word, "north") == 0) {
        return DIRECTION_NORTH;
    }
    if (strcmp(word, "south") == 0) {
        return DIRECTION_SOUTH;
    }
    if (strcmp(word, "east") == 0) {
        return DIRECTION_EAST;
    }
    if (strcmp(word, "west") == 0) {
        return DIRECTION_WEST;
    }
    return -1;
}

/* Номер состояния по слову или -1. */
static int state_number(const char *word) {
    for (int i = 0; i < STATE_COUNT; ++i) {
        if (strcmp(word, STATE_NAMES[i]) == 0) {
            return i;
        }
    }
    return -1;
}

/* Читает название состояния и меняет его бит по адресу states. */
static void change_state(const char *command, unsigned int *states) {
    if (states == NULL) {
        return;
    }
    char word[COMMAND_SIZE];
    int number = -1;
    if (scanf("%15s", word) == 1) {
        number = state_number(word);
    }
    if (number < 0) {
        printf("No such state. Known: poisoned, blinded, invisible,\n");
        printf("petrified, burning, asleep\n");
        return;
    }
    unsigned int mask = 1u << number;
    if (strcmp(command, "on") == 0) {
        *states = *states | mask;
    } else if (strcmp(command, "off") == 0) {
        *states = *states & ~mask;
    } else {
        *states = *states ^ mask;
    }
    printf("Hero is %s: %s\n", STATE_NAMES[number], (*states & mask) != 0 ? "yes" : "no");
}

/* Шаг героя: координаты, золото и счётчик ходов меняются по адресам. */
static void command_move(char map[][MAP_COLS], int direction,
                         int *x, int *y, unsigned int *gold, int *turns) {
    if (!hero_move(map, x, y, direction)) {
        printf("There is a wall.\n");
        return;
    }
    ++*turns;
    if (hero_pick_gold(map, *x, *y, gold)) {
        printf("The hero picks up %d gold. Total: %u\n", GOLD_PER_PILE, *gold);
    }
    if (map_at(map, *x, *y) == TILE_EXIT) {
        printf("Here is the exit to the next floor. Going down comes later.\n");
    }
    map_draw(map, *x, *y);
}

static void describe_cell(char map[][MAP_COLS], int x, int y) {
    char tile = ' ';
    int walkable = 0;
    if (!map_cell_info(map, x, y, &tile, &walkable)) {
        printf("Cell (%d, %d) is outside the map.\n", x, y);
        return;
    }
    printf("Cell (%d, %d): symbol '%c', %s\n", x, y, tile,
           walkable ? "walkable" : "not walkable");
}

static void command_check(char map[][MAP_COLS], int x, int y) {
    int cells = 0;
    int depth = 0;
    int found = reach_exit(map, x, y, &cells, &depth);
    printf("%s\n", found ? "The exit is reachable." : "The exit is unreachable.");
    printf("Cells visited: %d, recursion depth: %d\n", cells, depth);
}

/* Читает урон и наносит его. Возвращает 0, если герой погиб. */
static int command_hit(int *hp, int hp_max) {
    int damage = 0;
    int ok = scanf("%9d", &damage) == 1 && damage > 0;
    skip_rest_of_line();
    if (!ok) {
        printf("hit needs a positive number, e.g. hit 7\n");
        return 1;
    }
    hero_damage(hp, damage);
    printf("The hero takes %d damage. Health: %d / %d\n", damage, *hp, hp_max);
    if (*hp == 0) {
        printf("The hero is dead.\n");
        return 0;
    }
    return 1;
}

static void command_stacktest(void) {
    int size = 0;
    int ok = scanf("%9d", &size) == 1 && size > 0 && size <= REACH_FIELD_MAX;
    skip_rest_of_line();
    if (!ok) {
        printf("Specify the field side from 1 to %d\n", REACH_FIELD_MAX);
        return;
    }
    printf("Walking an open field %d x %d...\n", size, size);
    fflush(stdout); /* при падении текст из буфера иначе пропал бы */
    printf("Recursion depth: %d\n", reach_open_field(size));
}

/* Вспомогательная функция для addr: её локальная переменная лежит
   в кадре стека глубже, чем переменные game_run. */
static void print_deeper_local(void) {
    int deeper = 0;
    printf("  local in a called function       %p\n", (void *)&deeper);
}

/* Адреса переменных из разных областей памяти. */
static void print_addresses(char map[][MAP_COLS], int *hp, int *turns) {
    /* Статическая локальная переменная живёт всё время работы программы,
       как глобальная, но видна только в этой функции. Лежит не в стеке. */
    static int calls_here = 0;
    const char *literal = "string literal";

    ++addr_calls;
    ++calls_here;
    printf("Stack - local variables, live until the function returns:\n");
    printf("  health (in game_run)             %p\n", (void *)hp);
    printf("  turn counter (in game_run)       %p\n", (void *)turns);
    printf("  pointer literal (here)           %p\n", (void *)&literal);
    print_deeper_local();
    printf("Static memory - lives for the whole run of the program:\n");
    printf("  addr_calls, outside functions    %p\n", (void *)&addr_calls);
    printf("  calls_here, static in a function %p\n", (void *)&calls_here);
    printf("Read-only - program constants:\n");
    printf("  the string literal itself        %p\n", (void *)literal);
    printf("Map (a local array in game_run, so also the stack):\n");
    printf("  map              %p\n", (void *)map);
    printf("  &map[0][0]       %p\n", (void *)&map[0][0]);
    printf("  &map[0][1]       %p  (+1: next column)\n", (void *)&map[0][1]);
    printf("  &map[1][0]       %p  (+%d: next row)\n", (void *)&map[1][0], MAP_COLS);
    printf("addr calls: %d\n", calls_here);
}

/* Выполняет одну команду. Возвращает 0, если игра закончилась.
   Параметров много — так выглядит код, в котором связанные данные
   ещё не собраны в структуру. Так оставлено намеренно: на практике 8
   они станут одной структурой. */
static int handle_command(const char *command, char map[][MAP_COLS],
                          int *x, int *y, int *hp, int hp_max, unsigned char attack,
                          unsigned int *gold, int depth, unsigned int *states, int *turns) {
    int direction = direction_number(command);
    if (direction >= 0) {
        command_move(map, direction, x, y, gold, turns);
    } else if (strcmp(command, "look") == 0) {
        map_draw(map, *x, *y);
    } else if (strcmp(command, "cell") == 0) {
        describe_cell(map, *x, *y);
    } else if (strcmp(command, "check") == 0) {
        command_check(map, *x, *y);
    } else if (strcmp(command, "status") == 0) {
        print_status(*hp, hp_max, attack, *gold, depth, *states);
    } else if (strcmp(command, "hit") == 0) {
        return command_hit(hp, hp_max);
    } else if (strcmp(command, "on") == 0 || strcmp(command, "off") == 0
               || strcmp(command, "toggle") == 0) {
        change_state(command, states);
    } else if (strcmp(command, "addr") == 0) {
        print_addresses(map, hp, turns);
    } else if (strcmp(command, "stacktest") == 0) {
        command_stacktest();
    } else if (strcmp(command, "quit") == 0) {
        return 0;
    } else {
        printf("Unknown command: %s\n", command);
        print_commands();
    }
    return 1;
}

void game_run(void) {
    /* Почему выбраны такие типы — см. практику 2. */
    char map[MAP_ROWS][MAP_COLS];
    int x = 1;
    int y = 1;
    int hp = 30;
    int hp_max = 30;
    unsigned char attack = 5;
    unsigned int gold = 0;
    int depth = 1;
    unsigned int states = 0;

    /* Имя массива передаётся без &: оно и так превращается в адрес. */
    map_init(map);
    printf("Welcome to the dungeon!\n\n");
    map_draw(map, x, y);
    printf("\n");
    print_commands();

    int turns = 0;
    int running = 1;
    while (running) {
        printf("\nTurn %d > ", turns);
        fflush(stdout);

        char command[COMMAND_SIZE];
        if (scanf("%15s", command) == 1) {
            running = handle_command(command, map, &x, &y, &hp, hp_max, attack,
                                     &gold, depth, &states, &turns);
        } else {
            running = 0;
        }
    }
    printf("\nGame over. Turns made: %d\n", turns);
}
