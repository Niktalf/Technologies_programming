#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "command.h"
#include "entities.h"
#include "game.h"
#include "level.h"
#include "map.h"
#include "reach.h"
#include "status.h"

/* Практика 8: всё состояние игры собрано в структуру Game, и обработчики
   команд получают её адрес. На практике 7 у handle_command было
   пятнадцать параметров; теперь их три: игра, слова и их число.

   Модули: map — карта; entities — герой и монстры; level — загрузка
   уровня из файла; reach — достижимость выхода; command — разбор строки;
   game — игровой цикл и команды. */

/* Названия состояний в порядке битов: элемент номер i описывает бит номер i. */
static const char *const STATE_NAMES[STATE_COUNT] = {
    "poisoned", "blinded", "invisible", "petrified", "burning", "asleep"
};

/* Сколько монстров показывает команда monsters: длинный список не нужен. */
#define MONSTERS_SHOWN 20

static void print_commands(void) {
    printf("Commands (short form in parentheses):\n");
    printf("  go <where>, north (n), south (s), east (e), west (w) - step\n");
    printf("  look (l) - map; check (c) - is the exit reachable; cell - what is underfoot\n");
    printf("  status (st), inventory (i), use <number> (u) - use an item\n");
    printf("  attack <where> (a) - hit a monster next to you; monsters (m) - list\n");
    printf("  level <file> - load a level; spawn <N>; stress <N>\n");
    printf("  hit <N>, on/off/toggle <state>, sizes\n");
    printf("  help (h), quit (q)\n");
}

/* Отрисовка: карта, поверх неё монстры и герой. Сама карта не меняется,
   поэтому игра передаётся указателем на const. */
static void game_draw(const Game *game) {
    const Map *map = &game->map;
    for (int y = 0; y < map->height; ++y) {
        for (int x = 0; x < map->width; ++x) {
            char shown = map_at(map, x, y);
            if (monsters_find(&game->monsters, x, y) >= 0) {
                shown = 'M';
            }
            if (x == game->hero.x && y == game->hero.y) {
                shown = '@';
            }
            putchar(shown);
        }
        putchar('\n');
    }
}

/* Функции печати получают только ту часть игры, которая им нужна:
   по параметру сразу видно, что print_status смотрит лишь на героя. */
static void print_status(const Hero *hero) {
    printf("Health: %d / %d (%.1f%%)\n", hero->hp, hero->hp_max,
           (double)hero->hp / (double)hero->hp_max * 100.0);
    printf("Attack: %u, gold: %u, floor: %d\n",
           (unsigned)hero->attack, hero->gold, hero->depth);
    printf("States:");
    int any = 0;
    for (int i = 0; i < STATE_COUNT; ++i) {
        if ((hero->states & (1u << i)) != 0) {
            printf(" %s", STATE_NAMES[i]);
            any = 1;
        }
    }
    printf("%s\n", any ? "" : " none");
}

static void print_inventory(const Hero *hero) {
    if (hero->item_count == 0) {
        printf("The bag is empty.\n");
        return;
    }
    for (int i = 0; i < hero->item_count; ++i) {
        printf("  %d. %s\n", i + 1, hero_item_name(hero->items[i]));
    }
}

static void print_monsters(const MonsterList *monsters) {
    printf("Monsters: %d, list capacity: %d, doublings: %d\n",
           monsters->count, monsters->capacity, monsters->growths);
    for (int i = 0; i < monsters->count && i < MONSTERS_SHOWN; ++i) {
        printf("  %d. (%d, %d), health %d\n", i + 1,
               monsters->items[i].x, monsters->items[i].y, monsters->items[i].hp);
    }
    if (monsters->count > MONSTERS_SHOWN) {
        printf("  ... and %d more\n", monsters->count - MONSTERS_SHOWN);
    }
}

/* Размеры структур: почему они передаются адресом. Копия Game при каждом
   вызове стоила бы десятки байт, а адрес занимает восемь. */
static void print_sizes(void) {
    printf("sizeof(Hero)    = %u bytes\n", (unsigned)sizeof(Hero));
    printf("sizeof(Monster) = %u bytes\n", (unsigned)sizeof(Monster));
    printf("sizeof(Map)     = %u bytes\n", (unsigned)sizeof(Map));
    printf("sizeof(Game)    = %u bytes\n", (unsigned)sizeof(Game));
    printf("sizeof(Game *)  = %u bytes\n", (unsigned)sizeof(Game *));
    printf("The hero's attack field takes 1 byte, but an int follows it: the compiler\n");
    printf("inserts alignment padding, so the sum of the fields is less than sizeof.\n");
}

/* Номер направления по слову или -1. Слово уже в нижнем регистре. */
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

/* Число из слова с понятным сообщением при ошибке. Возвращает 1 при успехе. */
static int number_argument(const char *word, const char *example, int *out) {
    int code = command_parse_number(word, out);
    if (code != NUMBER_OK) {
        printf("%s. Example: %s\n", command_number_error(code), example);
        return 0;
    }
    return 1;
}

/* Номер состояния по слову или -1. Слово уже в нижнем регистре. */
static int state_number(const char *word) {
    for (int i = 0; i < STATE_COUNT; ++i) {
        if (strcmp(word, STATE_NAMES[i]) == 0) {
            return i;
        }
    }
    return -1;
}

/* Команды on, off и toggle: ставят, снимают или переключают бит состояния. */
static void command_state(Hero *hero, const char *command, char *word) {
    if (word == NULL) {
        printf("Which state? Example: %s poisoned\n", command);
        return;
    }
    command_to_lower(word);
    int number = state_number(word);
    if (number < 0) {
        printf("No state %s. Known: poisoned, blinded, invisible,\n", word);
        printf("petrified, burning, asleep\n");
        return;
    }
    unsigned int mask = 1u << number;
    if (strcmp(command, "on") == 0) {
        hero->states = hero->states | mask;
    } else if (strcmp(command, "off") == 0) {
        hero->states = hero->states & ~mask;
    } else {
        hero->states = hero->states ^ mask;
    }
    printf("Hero is %s: %s\n", STATE_NAMES[number], (hero->states & mask) != 0 ? "yes" : "no");
}

/* Селит до n монстров в свободные клетки. Возвращает код: память может
   кончиться. Сколько поселено — через *out_placed. */
static Status spawn_monsters(Game *game, int n, int *out_placed) {
    *out_placed = 0;
    int attempts = 0;
    Status status = STATUS_OK;
    while (status == STATUS_OK && *out_placed < n && attempts < n * 20) {
        /* Две координаты — двумя инструкциями: порядок вычисления
           аргументов не определён, а так он один на всех компиляторах. */
        int x = 1 + rand() % (game->map.width - 2);
        int y = 1 + rand() % (game->map.height - 2);
        ++attempts;
        if (map_at(&game->map, x, y) == TILE_FLOOR
            && !(x == game->hero.x && y == game->hero.y)
            && monsters_find(&game->monsters, x, y) < 0) {
            status = monsters_add(&game->monsters, x, y, MONSTER_HP);
            if (status == STATUS_OK) {
                ++*out_placed;
            }
        }
    }
    return status;
}

/* Шаг героя. Монстр в клетке назначения загораживает путь. */
static void command_move(Game *game, int direction) {
    int tx = 0;
    int ty = 0;
    hero_target(&game->hero, direction, &tx, &ty);
    if (monsters_find(&game->monsters, tx, ty) >= 0) {
        printf("A monster blocks the way. Attack: attack <where>\n");
        return;
    }
    if (!hero_move(&game->hero, &game->map, direction)) {
        printf("There is a wall.\n");
        return;
    }
    ++game->turns;
    if (hero_pick_gold(&game->hero, &game->map)) {
        printf("The hero picks up %d gold. Total: %u\n", GOLD_PER_PILE, game->hero.gold);
    }
    if (map_at(&game->map, game->hero.x, game->hero.y) == TILE_EXIT) {
        printf("Here is the exit to the next floor. Going down comes later.\n");
    }
    game_draw(game);
}

static void command_attack(Game *game, char *argument) {
    int direction = direction_argument(argument, "attack");
    if (direction < 0) {
        return;
    }
    int tx = 0;
    int ty = 0;
    hero_target(&game->hero, direction, &tx, &ty);
    int index = monsters_find(&game->monsters, tx, ty);
    if (index < 0) {
        printf("Nobody there.\n");
        return;
    }
    ++game->turns;
    /* Указатель на элемент списка годится только до следующего изменения
       списка: удаление или рост могут его испортить. Здесь он живёт три строки. */
    Monster *target = &game->monsters.items[index];
    target->hp = target->hp - game->hero.attack;
    if (target->hp > 0) {
        printf("Hit! The monster has %d health left.\n", target->hp);
        return;
    }
    monsters_remove(&game->monsters, index);
    printf("The monster is defeated. Monsters left: %d\n", game->monsters.count);
}

static void command_cell(const Game *game) {
    int x = game->hero.x;
    int y = game->hero.y;
    char tile = ' ';
    int walkable = 0;
    if (!map_cell_info(&game->map, x, y, &tile, &walkable)) {
        printf("Cell (%d, %d) is outside the map.\n", x, y);
        return;
    }
    printf("Cell (%d, %d): symbol '%c', %s\n", x, y, tile,
           walkable ? "walkable" : "not walkable");
}

static void command_check(const Game *game) {
    int found = 0;
    int cells = 0;
    int depth = 0;
    Status status = reach_exit(&game->map, game->hero.x, game->hero.y,
                               &found, &cells, &depth);
    if (status != STATUS_OK) {
        printf("The check failed: %s\n", status_text(status));
        return;
    }
    printf("%s\n", found ? "The exit is reachable." : "The exit is unreachable.");
    printf("Cells visited: %d, recursion depth: %d\n", cells, depth);
}

/* Загрузка уровня во время игры. При ошибке level_load игру не трогает,
   поэтому она продолжается на прежнем уровне. */
static void command_level(Game *game, const char *path) {
    if (path == NULL) {
        printf("Which file? Example: level levels/cave.txt\n");
        return;
    }
    int line = 0;
    Status status = level_load(path, game, &line);
    if (status != STATUS_OK) {
        if (line > 0) {
            printf("Level %s not loaded: %s (line %d)\n", path, status_text(status), line);
        } else {
            printf("Level %s not loaded: %s\n", path, status_text(status));
        }
        printf("The game goes on at the previous level.\n");
        return;
    }
    printf("Level loaded %s: %d x %d, monsters %d\n", path,
           game->map.width, game->map.height, game->monsters.count);
    game_draw(game);
}

static void command_spawn(Game *game, const char *argument) {
    int number = 0;
    if (!number_argument(argument, "spawn 10", &number)) {
        return;
    }
    int placed = 0;
    Status status = spawn_monsters(game, number, &placed);
    printf("Monsters added: %d\n", placed);
    if (status != STATUS_OK) {
        printf("Could not add the rest: %s\n", status_text(status));
    }
}

/* Создаёт и уничтожает уровень с монстрами times раз. Если что-то
   не освобождается, потребление памяти растёт с каждым кругом.
   Временная игра — локальная структура; в куче только её карта и список. */
static void command_stress(const Game *game, const char *argument) {
    int times = 0;
    if (!number_argument(argument, "stress 100", &times)) {
        return;
    }
    int failures = 0;
    for (int i = 0; i < times; ++i) {
        Game temp;
        monsters_init(&temp.monsters);
        hero_init(&temp.hero, 1, 1);
        if (map_create(&temp.map, game->map.width, game->map.height) != STATUS_OK) {
            ++failures;
        } else {
            map_generate(&temp.map, (unsigned int)i);
            int placed = 0;
            if (spawn_monsters(&temp, 100, &placed) != STATUS_OK) {
                ++failures;
            }
        }
        monsters_destroy(&temp.monsters);
        map_destroy(&temp.map);
    }
    printf("Levels created and destroyed: %d, failures: %d\n", times - failures, failures);
}

static void command_use(Hero *hero, const char *argument) {
    int number = 0;
    if (!number_argument(argument, "use 1", &number)) {
        return;
    }
    if (number < 1 || number > hero->item_count) {
        printf("No item number %d in the bag. Items: %d\n", number, hero->item_count);
        return;
    }
    printf("The hero uses: %s\n", hero_item_name(hero->items[number - 1]));
    hero_use_item(hero, number);
    printf("Health: %d / %d\n", hero->hp, hero->hp_max);
}

/* Наносит урон из аргумента. Возвращает 0, если герой погиб. */
static int command_hit(Hero *hero, const char *argument) {
    int damage = 0;
    if (!number_argument(argument, "hit 7", &damage)) {
        return 1;
    }
    hero_damage(hero, damage);
    printf("The hero takes %d damage. Health: %d / %d\n", damage, hero->hp, hero->hp_max);
    if (hero->hp == 0) {
        printf("The hero is dead.\n");
        return 0;
    }
    return 1;
}

/* Выполняет одну команду. Возвращает 0, если игра закончилась. */
static int handle_command(Game *game, char *words[], int count) {
    const char *name = words[0];
    char *argument = count > 1 ? words[1] : NULL;
    int direction = step_direction(words, count);
    if (direction >= 0) {
        command_move(game, direction);
    } else if (direction == -2) {
        /* go с неверным направлением: сообщение уже напечатано */
    } else if (command_is(name, "attack", "a")) {
        command_attack(game, argument);
    } else if (command_is(name, "look", "l")) {
        game_draw(game);
    } else if (command_is(name, "monsters", "m")) {
        print_monsters(&game->monsters);
    } else if (command_is(name, "level", NULL)) {
        command_level(game, argument); /* имя файла — без перевода в нижний регистр */
    } else if (command_is(name, "spawn", NULL)) {
        command_spawn(game, argument);
    } else if (command_is(name, "stress", NULL)) {
        command_stress(game, argument);
    } else if (command_is(name, "cell", NULL)) {
        command_cell(game);
    } else if (command_is(name, "check", "c")) {
        command_check(game);
    } else if (command_is(name, "status", "st")) {
        print_status(&game->hero);
    } else if (command_is(name, "inventory", "i")) {
        print_inventory(&game->hero);
    } else if (command_is(name, "use", "u")) {
        command_use(&game->hero, argument);
    } else if (command_is(name, "hit", NULL)) {
        return command_hit(&game->hero, argument);
    } else if (command_is(name, "on", NULL) || command_is(name, "off", NULL)
               || command_is(name, "toggle", NULL)) {
        command_state(&game->hero, name, argument);
    } else if (command_is(name, "sizes", NULL)) {
        print_sizes();
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
   строится заново со следующим зерном — до ста попыток. */
static Status create_random_level(Game *game, int width, int height, unsigned int seed) {
    for (int attempt = 0; attempt < 100; ++attempt) {
        Status status = map_create(&game->map, width, height);
        if (status != STATUS_OK) {
            return status; /* неверный размер или нет памяти: другое зерно не поможет */
        }
        map_generate(&game->map, seed + (unsigned int)attempt);
        int found = 0;
        status = reach_exit(&game->map, 1, 1, &found, NULL, NULL);
        if (status == STATUS_OK && found) {
            return STATUS_OK;
        }
        map_destroy(&game->map); /* неудачная карта освобождается перед новой попыткой */
        if (status != STATUS_OK) {
            return status;
        }
    }
    return STATUS_BAD_SIZE; /* на таком размере проходимый уровень не получился */
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

/* Спрашивает размер случайного уровня, пока ответ негоден. Возвращает 1,
   если уровень построен, и 0, если ввод кончился. */
static int ask_random_level(Game *game) {
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
        } else {
            Status result = create_random_level(game, width, height, (unsigned int)seed);
            if (result == STATUS_OK) {
                created = 1;
            } else {
                printf("Level not built: %s.\n\n", status_text(result));
            }
        }
    }
    return created;
}

/* Готовит первый уровень: из файла или случайный. Возвращает 0, если
   играть не на чем. Ошибка файла, заданного при запуске, — ошибка запуска,
   поэтому она уходит в stderr. */
static int prepare_level(Game *game, const char *level_path) {
    if (level_path != NULL) {
        int bad_line = 0;
        Status status = level_load(level_path, game, &bad_line);
        if (status != STATUS_OK) {
            fprintf(stderr, "Level %s not loaded: %s", level_path, status_text(status));
            if (bad_line > 0) {
                fprintf(stderr, " (line %d)", bad_line);
            }
            fprintf(stderr, ".\n");
            return 0;
        }
        return 1;
    }
    if (!ask_random_level(game)) {
        return 0;
    }
    int placed = 0;
    if (spawn_monsters(game, game->map.width * game->map.height / 100 + 1, &placed)
        != STATUS_OK) {
        printf("Fewer monsters than planned: not enough memory.\n");
    }
    return 1;
}

int game_run(const char *level_path) {
    /* Пустая игра: карты ещё нет, список монстров пуст. Освобождать такую
       игру безопасно, поэтому выход в конце один на все случаи. */
    Game game;
    game.map.cells = NULL;
    game.map.width = 0;
    game.map.height = 0;
    monsters_init(&game.monsters);
    hero_init(&game.hero, 1, 1);
    game.turns = 0;

    printf("Welcome to the dungeon!\n\n");
    int loaded = prepare_level(&game, level_path);
    if (!loaded) {
        monsters_destroy(&game.monsters);
        map_destroy(&game.map);
        printf("\nGoodbye.\n");
        return level_path != NULL ? 1 : 0; /* конец ввода при вопросе о размере — не ошибка */
    }

    printf("\n");
    game_draw(&game);
    printf("\n");
    print_commands();

    int running = 1;
    while (running) {
        printf("\nTurn %d > ", game.turns);
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
                /* В нижний регистр — только название команды. Аргументы
                   переводятся там, где их сравнивают с ключевыми словами:
                   имя файла уровня менять нельзя. */
                command_to_lower(words[0]);
                running = handle_command(&game, words, count);
            }
        }
    }
    printf("\nGame over. Turns made: %d\n", game.turns);

    /* Каждому выделению — ровно одно освобождение. */
    monsters_destroy(&game.monsters);
    map_destroy(&game.map);
    return 0;
}
