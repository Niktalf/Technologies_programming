#include <stdio.h>
#include <string.h>

#include "game.h"
#include "hero.h"
#include "map.h"
#include "reach.h"

/* Практика 4: главный цикл разложен на функции. Каждая функция делает
   одно дело, и её имя говорит какое.

   Состояние героя пока лежит в переменных этого файла, и функции меняют
   его напрямую. Так проще всего начать, но так функции связаны скрытой
   зависимостью: по заголовку try_move не видно, что она двигает героя.
   На практике 5 эти переменные станут локальными, а функции будут
   получать их адреса. */

#define COMMAND_SIZE  16
#define GOLD_PER_PILE 10

/* Названия состояний в порядке битов: элемент номер i описывает бит номер i. */
static const char *const STATE_NAMES[STATE_COUNT] = {
    "poisoned", "blinded", "invisible", "petrified", "burning", "asleep"
};

/* Почему выбраны такие типы — см. практику 2. */
static int hero_x = 1;
static int hero_y = 1;
static int hero_hp = 30;
static int hero_hp_max = 30;
static unsigned char hero_attack = 5;
static unsigned int hero_gold = 0;
static int hero_depth = 1;
static unsigned int hero_states = 0;
static int turns = 0;

static void print_commands(void) {
    printf("Commands: north, south, east, west - step; look - map;\n");
    printf("         check - is the exit reachable; status, hit <N>,\n");
    printf("         on/off/toggle <state>, stacktest <N>, quit\n");
    printf("States: poisoned, blinded, invisible, petrified, burning, asleep\n");
}

static void skip_rest_of_line(void) {
    int c = getchar();
    while (c != '\n' && c != EOF) {
        c = getchar();
    }
}

static void print_status(void) {
    printf("Health: %d / %d (%.1f%%)\n", hero_hp, hero_hp_max,
           (double)hero_hp / (double)hero_hp_max * 100.0);
    printf("Attack: %u, gold: %u, floor: %d\n", (unsigned)hero_attack, hero_gold, hero_depth);
    printf("States:");
    int any = 0;
    for (int i = 0; i < STATE_COUNT; ++i) {
        if ((hero_states & (1u << i)) != 0) {
            printf(" %s", STATE_NAMES[i]);
            any = 1;
        }
    }
    printf("%s\n", any ? "" : " none");
}

/* Переводит слово направления в сдвиг. Возвращает 1, если слово
   оказалось направлением. Сдвигов два, а вернуть функция может одно
   значение, поэтому пока сдвиг кодируется номером направления:
   0 — север, 1 — юг, 2 — восток, 3 — запад. */
static int direction_number(const char *word) {
    if (strcmp(word, "north") == 0) {
        return 0;
    }
    if (strcmp(word, "south") == 0) {
        return 1;
    }
    if (strcmp(word, "east") == 0) {
        return 2;
    }
    if (strcmp(word, "west") == 0) {
        return 3;
    }
    return -1;
}

/* Пытается сделать шаг. Возвращает 1, если шаг сделан. */
static int try_move(int direction) {
    static const int DX[4] = { 0, 0, 1, -1 };
    static const int DY[4] = { -1, 1, 0, 0 };
    int nx = hero_x + DX[direction];
    int ny = hero_y + DY[direction];
    if (!map_inside(nx, ny)) {
        printf("No way further: edge of the map.\n");
        return 0;
    }
    if (map_at(nx, ny) == TILE_WALL) {
        printf("There is a wall.\n");
        return 0;
    }
    hero_x = nx;
    hero_y = ny;
    if (map_at(nx, ny) == TILE_GOLD) {
        hero_gold = hero_gold + GOLD_PER_PILE;
        map_set(nx, ny, TILE_FLOOR);
        printf("The hero picks up %d gold. Total: %u\n", GOLD_PER_PILE, hero_gold);
    }
    if (map_at(nx, ny) == TILE_EXIT) {
        printf("Here is the exit to the next floor. Going down comes later.\n");
    }
    return 1;
}

/* Читает число урона и наносит его. Возвращает 0, если герой погиб. */
static int do_hit(void) {
    int damage = 0;
    int ok = scanf("%9d", &damage) == 1 && damage > 0;
    skip_rest_of_line();
    if (!ok) {
        printf("hit needs a positive number, e.g. hit 7\n");
        return 1;
    }
    hero_hp = hero_hp - damage;
    if (hero_hp < 0) {
        hero_hp = 0;
    }
    printf("The hero takes %d damage. Health: %d / %d\n", damage, hero_hp, hero_hp_max);
    if (hero_hp == 0) {
        printf("The hero is dead.\n");
        return 0;
    }
    return 1;
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

/* Команды on, off, toggle: читает название состояния и меняет его бит. */
static void change_state(const char *command) {
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
        hero_states = hero_states | mask;
    } else if (strcmp(command, "off") == 0) {
        hero_states = hero_states & ~mask;
    } else {
        hero_states = hero_states ^ mask;
    }
    printf("Hero is %s: %s\n", STATE_NAMES[number], (hero_states & mask) != 0 ? "yes" : "no");
}

static void check_exit(void) {
    if (reach_exit(hero_x, hero_y)) {
        printf("The exit is reachable.\n");
    } else {
        printf("The exit is unreachable.\n");
    }
    printf("Cells visited: %d, recursion depth: %d\n",
           reach_last_cells(), reach_last_depth());
}

static void stack_test(void) {
    int size = 0;
    int ok = scanf("%9d", &size) == 1 && size > 0 && size <= REACH_FIELD_MAX;
    skip_rest_of_line();
    if (!ok) {
        printf("Specify the field side from 1 to %d, e.g. stacktest 300\n", REACH_FIELD_MAX);
        return;
    }
    printf("Walking an open field %d x %d...\n", size, size);
    fflush(stdout); /* при падении текст из буфера иначе пропал бы */
    printf("Recursion depth: %d\n", reach_open_field(size));
}

/* Выполняет одну команду. Возвращает 0, если игра закончилась. */
static int handle_command(const char *command) {
    int direction = direction_number(command);
    if (direction >= 0) {
        if (try_move(direction)) {
            ++turns;
            map_draw(hero_x, hero_y);
        }
    } else if (strcmp(command, "look") == 0) {
        map_draw(hero_x, hero_y);
    } else if (strcmp(command, "check") == 0) {
        check_exit();
    } else if (strcmp(command, "status") == 0) {
        print_status();
    } else if (strcmp(command, "hit") == 0) {
        return do_hit();
    } else if (strcmp(command, "on") == 0 || strcmp(command, "off") == 0
               || strcmp(command, "toggle") == 0) {
        change_state(command);
    } else if (strcmp(command, "stacktest") == 0) {
        stack_test();
    } else if (strcmp(command, "quit") == 0) {
        return 0;
    } else {
        printf("Unknown command: %s\n", command);
        print_commands();
    }
    return 1;
}

void game_run(void) {
    map_init();
    printf("Welcome to the dungeon!\n\n");
    map_draw(hero_x, hero_y);
    printf("\n");
    print_commands();

    int running = 1;
    while (running) {
        printf("\nTurn %d > ", turns);
        fflush(stdout);

        char command[COMMAND_SIZE];
        if (scanf("%15s", command) == 1) {
            running = handle_command(command);
        } else {
            running = 0;
        }
    }
    printf("\nGame over. Turns made: %d\n", turns);
}
