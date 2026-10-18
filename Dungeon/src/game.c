#include <stdio.h>
#include <string.h>

#include "command.h"
#include "game.h"
#include "hero.h"
#include "map.h"
#include "reach.h"

/* Практика 6: строка читается целиком и разбирается на слова вручную.
   Команды понимают сокращения: go north, north и n — одно и то же.

   Практика 5: глобальных переменных больше нет. Карта и характеристики
   героя — локальные переменные game_run, и функции получают их адреса.
   По списку параметров видно, что функция может изменить: всё, что
   передано указателем без const, и только это. */

/* Счётчик вызовов команды addr. Переменная вне функций живёт всё время
   работы программы. Нужна, чтобы показать, где лежат такие переменные. */
static int addr_calls = 0;

/* Названия состояний в порядке битов: элемент номер i описывает бит номер i. */
static const char *const STATE_NAMES[STATE_COUNT] = {
    "poisoned", "blinded", "invisible", "petrified", "burning", "asleep"
};

static void print_commands(void) {
    printf("Commands (short form in parentheses):\n");
    printf("  go <where>, north (n), south (s), east (e), west (w) - step\n");
    printf("  look (l) - map; check (c) - is the exit reachable; cell - what is underfoot\n");
    printf("  status (st), inventory (i), use <number> (u) - use an item\n");
    printf("  hit <N>, on/off/toggle <state>, addr, stacktest <N>\n");
    printf("  help (h), quit (q)\n");
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

/* Номер направления по слову или -1. Регистр не важен: слово уже
   переведено в нижний регистр. */
static int direction_number(const char *word) {
    if (command_is(word, "north", "n")) {
        return DIRECTION_NORTH;
    }
    if (command_is(word, "south", "s")) {
        return DIRECTION_SOUTH;
    }
    if (command_is(word, "east", "e")) {
        return DIRECTION_EAST;
    }
    if (command_is(word, "west", "w")) {
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

/* Меняет бит состояния по адресу states. Название состояния — слово word. */
static void change_state(const char *command, const char *word, unsigned int *states) {
    if (states == NULL) {
        return;
    }
    if (word == NULL) {
        printf("Which state? Example: %s poisoned\n", command);
        return;
    }
    int number = state_number(word);
    if (number < 0) {
        printf("No state %s. Known: poisoned, blinded, invisible,\n", word);
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

/* Число из слова с понятным сообщением при ошибке. Возвращает 1 при успехе. */
static int number_argument(const char *word, const char *example, int *out) {
    int code = command_parse_number(word, out);
    if (code != NUMBER_OK) {
        printf("%s. Example: %s\n", command_number_error(code), example);
        return 0;
    }
    return 1;
}

static void print_inventory(const int items[], int item_count) {
    if (item_count == 0) {
        printf("The bag is empty.\n");
        return;
    }
    for (int i = 0; i < item_count; ++i) {
        printf("  %d. %s\n", i + 1, hero_item_name(items[i]));
    }
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

/* Наносит урон из аргумента. Возвращает 0, если герой погиб. */
static int command_hit(const char *argument, int *hp, int hp_max) {
    int damage = 0;
    if (!number_argument(argument, "hit 7", &damage)) {
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

static void command_use(const char *argument, int items[], int *item_count,
                        int *hp, int hp_max, unsigned int *states) {
    int number = 0;
    if (!number_argument(argument, "use 1", &number)) {
        return;
    }
    if (number < 1 || number > *item_count) {
        printf("No item number %d in the bag. Items: %d\n", number, *item_count);
        return;
    }
    printf("The hero uses: %s\n", hero_item_name(items[number - 1]));
    hero_use_item(items, item_count, number, hp, hp_max, states);
    printf("Health: %d / %d\n", *hp, hp_max);
}

static void command_stacktest(const char *argument) {
    int size = 0;
    if (!number_argument(argument, "stacktest 300", &size)) {
        return;
    }
    if (size < 1 || size > REACH_FIELD_MAX) {
        printf("The field side is from 1 to %d\n", REACH_FIELD_MAX);
        return;
    }
    printf("Walking an open field %d x %d...\n", size, size);
    fflush(stdout); /* при падении текст из буфера иначе пропал бы */
    printf("Recursion depth: %d\n", reach_open_field(size));
}

/* Направление шага: из самого слова (north, n) или из второго слова
   после go. Возвращает -1, если команда — не шаг; -2, если это go
   с неверным направлением (сообщение уже напечатано). */
static int step_direction(char *words[], int count) {
    if (!command_is(words[0], "go", NULL)) {
        return direction_number(words[0]);
    }
    if (count < 2) {
        printf("Go where? Example: go north\n");
        return -2;
    }
    command_to_lower(words[1]);
    int direction = direction_number(words[1]);
    if (direction < 0) {
        printf("Unknown direction: %s. Known: north, south, east, west.\n", words[1]);
        return -2;
    }
    return direction;
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

/* Выполняет одну команду, разобранную на слова. Возвращает 0, если игра
   закончилась. Параметров уже четырнадцать: связанные данные героя
   по-прежнему лежат отдельными переменными. Так оставлено намеренно:
   на практике 8 они станут одной структурой. */
static int handle_command(char *words[], int count, char map[][MAP_COLS],
                          int *x, int *y, int *hp, int hp_max, unsigned char attack,
                          unsigned int *gold, int depth, unsigned int *states, int *turns,
                          int items[], int *item_count) {
    const char *name = words[0];
    const char *argument = count > 1 ? words[1] : NULL;
    int direction = step_direction(words, count);
    if (direction >= 0) {
        command_move(map, direction, x, y, gold, turns);
    } else if (direction == -2) {
        /* go с неверным направлением: сообщение уже напечатано */
    } else if (command_is(name, "look", "l")) {
        map_draw(map, *x, *y);
    } else if (command_is(name, "cell", NULL)) {
        describe_cell(map, *x, *y);
    } else if (command_is(name, "check", "c")) {
        command_check(map, *x, *y);
    } else if (command_is(name, "status", "st")) {
        print_status(*hp, hp_max, attack, *gold, depth, *states);
    } else if (command_is(name, "inventory", "i")) {
        print_inventory(items, *item_count);
    } else if (command_is(name, "use", "u")) {
        command_use(argument, items, item_count, hp, hp_max, states);
    } else if (command_is(name, "hit", NULL)) {
        return command_hit(argument, hp, hp_max);
    } else if (command_is(name, "on", NULL) || command_is(name, "off", NULL)
               || command_is(name, "toggle", NULL)) {
        /* Название состояния сравнивается со словами в нижнем регистре. */
        if (count > 1) {
            command_to_lower(words[1]);
        }
        change_state(name, argument, states);
    } else if (command_is(name, "addr", NULL)) {
        print_addresses(map, hp, turns);
    } else if (command_is(name, "stacktest", NULL)) {
        command_stacktest(argument);
    } else if (command_is(name, "help", "h")) {
        print_commands();
    } else if (command_is(name, "quit", "q")) {
        return 0;
    } else {
        printf("Unknown command: %s. Type help.\n", name);
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
    /* Сумка: в начале четыре предмета, остальные места пустые. */
    int items[INVENTORY_SIZE] = { ITEM_POTION, ITEM_ANTIDOTE, ITEM_TORCH, ITEM_POTION };
    int item_count = 4;

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

        char line[LINE_SIZE];
        int status = command_read_line(line, (int)sizeof line);
        if (status == READ_END) {
            running = 0;
        } else if (status == READ_TOO_LONG) {
            printf("Line too long: at most %d characters.\n", LINE_SIZE - 2);
        } else {
            char *words[WORDS_MAX];
            int count = command_split(line, words, WORDS_MAX);
            if (count > 0) {
                /* В нижний регистр — только название команды: NORTH и north
                   одно и то же. Аргументы-слова переводятся там, где их
                   сравнивают с ключевыми словами. */
                command_to_lower(words[0]);
                running = handle_command(words, count, map, &x, &y, &hp, hp_max, attack,
                                         &gold, depth, &states, &turns, items, &item_count);
            }
        }
    }
    printf("\nGame over. Turns made: %d\n", turns);
}
