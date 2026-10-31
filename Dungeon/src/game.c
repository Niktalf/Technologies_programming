#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "command.h"
#include "game.h"
#include "hero.h"
#include "map.h"
#include "monsters.h"
#include "reach.h"

/* Практика 7: размер карты задаётся при запуске, карта и список монстров
   живут в куче. Всё выделенное освобождается перед выходом.

   Практика 6: строка читается целиком и разбирается на слова вручную.
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
    printf("  attack <where> (a) - hit a monster next to you; monsters (m) - list\n");
    printf("  spawn <N> - add monsters; stress <N> - create and destroy a level N times\n");
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

#define MONSTER_HP 8

/* Шаг героя: координаты, золото и счётчик ходов меняются по адресам.
   Монстр в клетке назначения загораживает путь. */
static void command_move(Map *map, const Monsters *monsters, int direction,
                         int *x, int *y, unsigned int *gold, int *turns) {
    int tx = 0;
    int ty = 0;
    hero_target(*x, *y, direction, &tx, &ty);
    if (map_inside(map, tx, ty) && monsters_find(monsters, map_index(map, ty, tx)) >= 0) {
        printf("A monster blocks the way. Attack: attack <where>\n");
        return;
    }
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
    map_draw(map, *x, *y, monsters->cells, monsters->count);
}

static void describe_cell(const Map *map, int x, int y) {
    char tile = ' ';
    int walkable = 0;
    if (!map_cell_info(map, x, y, &tile, &walkable)) {
        printf("Cell (%d, %d) is outside the map.\n", x, y);
        return;
    }
    printf("Cell (%d, %d): symbol '%c', %s\n", x, y, tile,
           walkable ? "walkable" : "not walkable");
}

static void command_check(const Map *map, int x, int y) {
    int cells = 0;
    int depth = 0;
    int found = reach_exit(map, x, y, &cells, &depth);
    if (found < 0) {
        printf("Not enough memory for the check.\n");
        return;
    }
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

/* Направление из слова-аргумента (с переводом в нижний регистр) или -1
   с сообщением. */
static int direction_argument(char *word, const char *command) {
    if (word == NULL) {
        printf("Where? Example: %s north\n", command);
        return -1;
    }
    command_to_lower(word);
    int direction = direction_number(word);
    if (direction < 0) {
        printf("Unknown direction: %s. Known: north, south, east, west.\n", word);
    }
    return direction;
}

/* Направление шага: из самого слова (north, n) или из второго слова
   после go. Возвращает -1, если команда — не шаг; -2, если это go
   с неверным направлением (сообщение уже напечатано). */
static int step_direction(char *words[], int count) {
    if (!command_is(words[0], "go", NULL)) {
        return direction_number(words[0]);
    }
    int direction = direction_argument(count > 1 ? words[1] : NULL, "go");
    return direction >= 0 ? direction : -2;
}

/* Селит n монстров в случайные свободные клетки. Возвращает, сколько
   удалось поселить: место или память могут кончиться раньше. */
static int spawn_monsters(const Map *map, Monsters *monsters, int n, int hero_x, int hero_y) {
    int placed = 0;
    int attempts = 0;
    int out_of_memory = 0;
    while (placed < n && attempts < n * 20 && !out_of_memory) {
        /* Две координаты — двумя инструкциями: порядок вычисления
           аргументов функции не определён. */
        int x = 1 + rand() % (map->width - 2);
        int y = 1 + rand() % (map->height - 2);
        int cell = map_index(map, y, x);
        ++attempts;
        int free_cell = map_at(map, x, y) == TILE_FLOOR && !(x == hero_x && y == hero_y)
                        && monsters_find(monsters, cell) < 0;
        if (free_cell) {
            if (monsters_add(monsters, cell, MONSTER_HP)) {
                ++placed;
            } else {
                printf("Not enough memory for a new monster.\n");
                out_of_memory = 1;
            }
        }
    }
    return placed;
}

static void print_monsters(const Map *map, const Monsters *monsters) {
    printf("Monsters: %d, list capacity: %d, doublings: %d\n",
           monsters->count, monsters->capacity, monsters->growths);
    for (int i = 0; i < monsters->count && i < 20; ++i) {
        printf("  %d. (%d, %d), health %d\n", i + 1,
               monsters->cells[i] % map->width, monsters->cells[i] / map->width,
               monsters->hps[i]);
    }
    if (monsters->count > 20) {
        printf("  ... and %d more\n", monsters->count - 20);
    }
}

/* Удар по монстру в соседней клетке. Побеждённый монстр удаляется из списка.
   Возвращает 1, если удар состоялся и ход потрачен. */
static int command_attack(const Map *map, Monsters *monsters, char *argument,
                          int x, int y, unsigned char power) {
    int direction = direction_argument(argument, "attack");
    if (direction < 0) {
        return 0;
    }
    int tx = 0;
    int ty = 0;
    hero_target(x, y, direction, &tx, &ty);
    int index = map_inside(map, tx, ty) ? monsters_find(monsters, map_index(map, ty, tx)) : -1;
    if (index < 0) {
        printf("Nobody there.\n");
        return 1;
    }
    monsters->hps[index] = monsters->hps[index] - power;
    if (monsters->hps[index] > 0) {
        printf("Hit! The monster has %d health left.\n", monsters->hps[index]);
        return 1;
    }
    monsters_remove(monsters, index);
    printf("The monster is defeated. Monsters left: %d\n", monsters->count);
    return 1;
}

static void command_spawn(const char *argument, const Map *map, Monsters *monsters,
                          int x, int y) {
    int number = 0;
    if (!number_argument(argument, "spawn 10", &number)) {
        return;
    }
    printf("Monsters added: %d\n", spawn_monsters(map, monsters, number, x, y));
    print_monsters(map, monsters);
}

/* Создаёт и уничтожает уровень с монстрами times раз. Если что-то
   не освобождается, потребление памяти растёт с каждым кругом. */
static void command_stress(const char *argument, int width, int height) {
    int times = 0;
    if (!number_argument(argument, "stress 100", &times)) {
        return;
    }
    int failures = 0;
    for (int i = 0; i < times; ++i) {
        Map map;
        if (map_create(&map, width, height, (unsigned int)i)) {
            Monsters monsters;
            monsters_init(&monsters);
            spawn_monsters(&map, &monsters, 100, 1, 1);
            monsters_destroy(&monsters);
            map_destroy(&map);
            /* Повторное освобождение безопасно: указатели обнулены. */
            map_destroy(&map);
            monsters_destroy(&monsters);
        } else {
            ++failures;
        }
    }
    printf("Levels created and destroyed: %d, failures: %d\n", times - failures, failures);
}

/* Вспомогательная функция для addr: её локальная переменная лежит
   в кадре стека глубже, чем переменные game_run. */
static void print_deeper_local(void) {
    int deeper = 0;
    printf("  local in a called function       %p\n", (void *)&deeper);
}

/* Адреса переменных из разных областей памяти. */
static void print_addresses(const Map *map, const Monsters *monsters, int *hp, int *turns) {
    /* Статическая локальная переменная живёт всё время работы программы,
       как глобальная, но видна только в этой функции. Лежит не в стеке. */
    static int calls_here = 0;
    const char *literal = "string literal";

    ++addr_calls;
    ++calls_here;
    printf("Stack - local variables, live until the function returns:\n");
    printf("  health (in game_run)             %p\n", (void *)hp);
    printf("  turn counter (in game_run)       %p\n", (void *)turns);
    printf("  map structure (in game_run)      %p\n", (void *)map);
    print_deeper_local();
    printf("Heap - memory from malloc and realloc, lives until free:\n");
    printf("  map cells                        %p\n", (void *)map->cells);
    printf("  monster cell array               %p\n", (void *)monsters->cells);
    printf("Static memory - lives for the whole run of the program:\n");
    printf("  addr_calls, outside functions    %p\n", (void *)&addr_calls);
    printf("  calls_here, static in a function %p\n", (void *)&calls_here);
    printf("Read-only - program constants:\n");
    printf("  the string literal itself        %p\n", (void *)literal);
    printf("Map cells: row 0 column 1 is +1, row 1 column 0 is +%d (the width)\n",
           map->width);
    printf("addr calls: %d\n", calls_here);
}

/* Выполняет одну команду, разобранную на слова. Возвращает 0, если игра
   закончилась. Параметров уже пятнадцать: связанные данные героя
   по-прежнему лежат отдельными переменными. Так оставлено намеренно:
   на практике 8 герой и всё состояние игры станут структурами. */
static int handle_command(char *words[], int count, Map *map, Monsters *monsters,
                          int *x, int *y, int *hp, int hp_max, unsigned char power,
                          unsigned int *gold, int depth, unsigned int *states, int *turns,
                          int items[], int *item_count) {
    const char *name = words[0];
    char *argument = count > 1 ? words[1] : NULL;
    int direction = step_direction(words, count);
    if (direction >= 0) {
        command_move(map, monsters, direction, x, y, gold, turns);
    } else if (direction == -2) {
        /* go с неверным направлением: сообщение уже напечатано */
    } else if (command_is(name, "attack", "a")) {
        if (command_attack(map, monsters, argument, *x, *y, power)) {
            ++*turns;
        }
    } else if (command_is(name, "look", "l")) {
        map_draw(map, *x, *y, monsters->cells, monsters->count);
    } else if (command_is(name, "monsters", "m")) {
        print_monsters(map, monsters);
    } else if (command_is(name, "spawn", NULL)) {
        command_spawn(argument, map, monsters, *x, *y);
    } else if (command_is(name, "stress", NULL)) {
        command_stress(argument, map->width, map->height);
    } else if (command_is(name, "cell", NULL)) {
        describe_cell(map, *x, *y);
    } else if (command_is(name, "check", "c")) {
        command_check(map, *x, *y);
    } else if (command_is(name, "status", "st")) {
        print_status(*hp, hp_max, power, *gold, depth, *states);
    } else if (command_is(name, "inventory", "i")) {
        print_inventory(items, *item_count);
    } else if (command_is(name, "use", "u")) {
        command_use(argument, items, item_count, hp, hp_max, states);
    } else if (command_is(name, "hit", NULL)) {
        return command_hit(argument, hp, hp_max);
    } else if (command_is(name, "on", NULL) || command_is(name, "off", NULL)
               || command_is(name, "toggle", NULL)) {
        /* Название состояния сравнивается со словами в нижнем регистре. */
        if (argument != NULL) {
            command_to_lower(argument);
        }
        change_state(name, argument, states);
    } else if (command_is(name, "addr", NULL)) {
        print_addresses(map, monsters, hp, turns);
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

/* Случайные перегородки иногда замуровывают старт или выход. Тогда уровень
   строится заново со следующим зерном — до ста попыток. Возвращает 1,
   если построена карта, где выход достижим из (1, 1). */
static int create_passable_map(Map *map, int width, int height, unsigned int seed) {
    for (int attempt = 0; attempt < 100; ++attempt) {
        if (!map_create(map, width, height, seed + (unsigned int)attempt)) {
            return 0; /* неверный размер или нет памяти: другое зерно не поможет */
        }
        if (reach_exit(map, 1, 1, NULL, NULL) == 1) {
            return 1;
        }
        map_destroy(map); /* неудачная карта освобождается перед новой попыткой */
    }
    return 0;
}

/* Разбирает ответ «ширина высота [зерно]». Возвращает 1, если в нём
   два или три числа. */
static int parse_size(char *words[], int count, int *width, int *height, int *seed) {
    if (count < 2 || count > 3) {
        return 0;
    }
    if (command_parse_number(words[0], width) != NUMBER_OK
        || command_parse_number(words[1], height) != NUMBER_OK) {
        return 0;
    }
    return count < 3 || command_parse_number(words[2], seed) == NUMBER_OK;
}

/* Спрашивает размеры карты: «ширина высота [зерно]». Пока ответ негоден,
   переспрашивает. Возвращает 1, если карта создана, и 0, если ввод кончился. */
static int ask_and_create_map(Map *map) {
    int created = 0;
    int input_ended = 0;
    while (!created && !input_ended) {
        printf("Dungeon size: width and height, optionally a seed\n");
        printf("(width %d-%d, height %d-%d, e.g. 40 15 7): ",
               MAP_MIN_SIDE, MAP_MAX_WIDTH, MAP_MIN_SIDE, MAP_MAX_HEIGHT);
        fflush(stdout);

        char line[LINE_SIZE];
        char *words[WORDS_MAX];
        int count = 0;
        int status = command_read_line(line, (int)sizeof line);
        if (status == READ_OK) {
            count = command_split(line, words, WORDS_MAX);
        }
        int width = 0;
        int height = 0;
        int seed = 1;
        if (status == READ_END) {
            input_ended = 1;
        } else if (!parse_size(words, count, &width, &height, &seed)) {
            printf("Two or three numbers separated by spaces are needed.\n\n");
        } else if (width < MAP_MIN_SIDE || height < MAP_MIN_SIDE
                   || width > MAP_MAX_WIDTH || height > MAP_MAX_HEIGHT) {
            printf("This size is not allowed.\n\n");
        } else if (!create_passable_map(map, width, height, (unsigned int)seed)) {
            printf("Cannot build a map %d x %d.\n\n", width, height);
        } else {
            created = 1;
        }
    }
    return created;
}

void game_run(void) {
    printf("Welcome to the dungeon!\n\n");
    Map map;
    if (!ask_and_create_map(&map)) {
        printf("\nGoodbye.\n");
        return;
    }
    Monsters monsters;
    monsters_init(&monsters);
    spawn_monsters(&map, &monsters, map.width * map.height / 100 + 1, 1, 1);

    /* Почему выбраны такие типы — см. практику 2. */
    int x = 1;
    int y = 1;
    int hp = 30;
    int hp_max = 30;
    unsigned char power = 5;
    unsigned int gold = 0;
    int depth = 1;
    unsigned int states = 0;
    /* Сумка: в начале четыре предмета, остальные места пустые. */
    int items[INVENTORY_SIZE] = { ITEM_POTION, ITEM_ANTIDOTE, ITEM_TORCH, ITEM_POTION };
    int item_count = 4;

    printf("\n");
    map_draw(&map, x, y, monsters.cells, monsters.count);
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
                running = handle_command(words, count, &map, &monsters, &x, &y, &hp, hp_max,
                                         power, &gold, depth, &states, &turns,
                                         items, &item_count);
            }
        }
    }
    printf("\nGame over. Turns made: %d\n", turns);

    /* Каждому выделению — ровно одно освобождение. */
    monsters_destroy(&monsters);
    map_destroy(&map);
}
