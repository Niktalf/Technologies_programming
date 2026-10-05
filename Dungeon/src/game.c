#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "command.h"
#include "game.h"
#include "hero.h"
#include "map.h"
#include "reach.h"

#define TRAP_DAMAGE 7
#define REST_HEAL   4

static void print_help() {
    printf("Available commands:\n");
    printf("  look          - watch\n");
    printf("  go <where>    - step: go north, go n, go east\n");
    printf("  take <what>   - pick up: take gold\n");
    printf("  attack <whom> - attack: attack goblin\n");
    printf("  status        - characteristics and status of the hero\n");
    printf("  nearest       - nearest free cell\n");
    printf("  demo          - copy vs address\n");
    printf("  reach         - is the ladder achievable from here\n");
    printf("  hit           - take damage from a trap\n");
    printf("  rest          - rest and restore health\n");
    printf("  poison        - switch the infection state\n");
    printf("  help          - this list\n");
    printf("  quit          - get out of the game\n");
}

static void damage_copy(Hero hero, const int amount) {
    hero.hp -= amount;
}

static void damage_pointer(Hero *hero, const int amount) {
    if (hero == NULL) {
        return;
    }
    hero->hp -= amount;
}

static void demo_copy_vs_pointer(Hero *hero) {
    const int before = hero->hp;

    damage_copy(*hero, 5);
    printf("After the function that received the copy: health  %d (was %d)\n", hero->hp, before);

    damage_pointer(hero, 5);
    printf("After the function that received the address: health %d (was %d)\n", hero->hp, before);

    hero->hp = before;
    printf("Size of the hero structure: %u bytes, size of the pointer to it: %u bytes\n",
           (unsigned)sizeof(Hero), (unsigned)sizeof(Hero *));
}

static void take_gold(Hero *hero) {
    if (map_at(hero->x, hero->y) != TILE_GOLD) {
        printf("There's nothing to pick up here.\n");
        return;
    }
    hero_add_gold(hero, 10);
    map_set(hero->x, hero->y, TILE_FLOOR);
    printf("You have picked up 10 gold. Total: %u\n", hero->gold);
}

static void show_nearest(const Hero *hero) {
    int x = 0;
    int y = 0;

    if (map_find_free(hero->x, hero->y, &x, &y)) {
        printf("Nearest available cell: (%d, %d)\n", x, y);
    } else {
        printf("There are no free cells.\n");
    }
}

static void print_reach(const Hero *hero) {
    const ReachReport report = reach_check(hero->x, hero->y);

    printf("Ladder %s.\n", report.stairs_reachable ? "attainable" : "unattainable");
    printf("Achievable cells: %d, recursion depth: %d\n",
           report.cells, report.max_depth);
}

static int try_move(Hero *hero, const int dx, const int dy) {
    const int nx = hero->x + dx;
    const int ny = hero->y + dy;

    if (!map_walkable(nx, ny)) {
        printf("There's a wall.\n");
        return 0;
    }
    hero->x = nx;
    hero->y = ny;

    if (map_at(nx, ny) == TILE_STAIRS) {
        printf("You are standing on the stairs going down. The descent to the next floor will appear later.\n");
    }
    return 1;
}

static int same_word(const char *a, const char *b) {
    if (a == NULL || b == NULL) {
        return 0;
    }

    int i;
    for (i = 0; a[i] != '\0' && b[i] != '\0'; ++i) {
        if (tolower((unsigned char)a[i]) != tolower((unsigned char)b[i])) {
            return 0;
        }
    }
    return a[i] == '\0' && b[i] == '\0';
}

static int direction_from_word(const char *word, int *dx, int *dy) {
    if (word == NULL) {
        return 0;
    }
    if (same_word(word, "north") || same_word(word, "n")) {
        *dx = 0; *dy = -1; return 1;
    }
    if (same_word(word, "south") || same_word(word, "s")) {
        *dx = 0; *dy = 1; return 1;
    }
    if (same_word(word, "east") || same_word(word, "e")) {
        *dx = 1; *dy = 0; return 1;
    }
    if (same_word(word, "west") || same_word(word, "w")) {
        *dx = -1; *dy = 0; return 1;
    }
    return 0;
}

static void do_go(Hero *hero, const Command *command) {
    const char *where = command_argument(command, 1);
    int dx = 0;
    int dy = 0;

    /* Команда без обязательного аргумента — обычное дело, а не авария. */
    if (where == NULL) {
        printf("Where to go? For example: go north\n");
        return;
    }
    if (!direction_from_word(where, &dx, &dy)) {
        printf("I don't know the direction of \"%s\". There are north, south, east, west.\n", where);
        return;
    }
    if (try_move(hero, dx, dy)) {
        map_draw(hero->x, hero->y);
    }
}

static void do_take(Hero *hero, const Command *command) {
    const char *what = command_argument(command, 1);

    if (what != NULL && !same_word(what, "gold")) {
        printf("Not here: %s\n", what);
        return;
    }
    take_gold(hero);
}

static void do_attack(const Command *command) {
    const char *target = command_argument(command, 1);

    if (target == NULL) {
        printf("Who should I attack? For example: attack goblin\n");
        return;
    }
    printf("There is no one named \"%s\".\n", target);
}

static int handle(Hero *hero, const Command *command) {
    const char *name = command_name(command);

    if (command->count == 0) {
        return 1;
    }
    if (command->count > 2) {
        printf("Extra words after \"%s %s\"  are not needed, but I won't notice them.\n",
            name, command_argument(command, 1));
    }

    if (strcmp(name, "look") == 0) {
        map_draw(hero->x, hero->y);
    } else if (strcmp(name, "go") == 0) {
        do_go(hero, command);
    } else if (strcmp(name, "take") == 0) {
        do_take(hero, command);
    } else if (strcmp(name, "attack") == 0) {
        do_attack(command);
    } else if (strcmp(name, "nearest") == 0) {
        show_nearest(hero);
    } else if (strcmp(name, "demo") == 0) {
        demo_copy_vs_pointer(hero);
    } else if (strcmp(name, "reach") == 0) {
        print_reach(hero);
    } else if (strcmp(name, "status") == 0) {
        hero_print_status(hero);
    } else if (strcmp(name, "hit") == 0) {
        hero_take_damage(hero, TRAP_DAMAGE);
        hero_state_set(hero, STATE_IN_FIGHT);
        printf("The trap hits you. Health: %d / %d\n", hero->hp, hero->hp_max);
        if (!hero_is_alive(hero)) {
            printf("You died in the dungeon.\n");
            return 0;
        }
    } else if (strcmp(name, "rest") == 0) {
        hero_heal(hero, REST_HEAL);
        hero_state_clear(hero, STATE_IN_FIGHT);
        printf("You are resting. Health: %d / %d\n", hero->hp, hero->hp_max);
    } else if (strcmp(name, "poison") == 0) {
        hero_state_toggle(hero, STATE_POISONED);
        printf("Status \"%s\": %s\n", hero_state_name(STATE_POISONED),
            hero_state_has(hero, STATE_POISONED) ? "enabled" : "disabled");
    } else if (strcmp(name, "help") == 0) {
        print_help();
    } else if (strcmp(name, "quit") == 0) {
        return 0;
    } else {
        printf("Unknown command: %s\n", name);
        printf("Type 'help' to see the list of commands.\n");
    }
    return 1;
}

static void play() {
    Hero hero;
    Command command;
    int running = 1;

    hero_init(&hero);
    map_init();

    printf("You're going down to the dungeon.\n\n");
    map_draw(hero.x, hero.y);
    print_reach(&hero);

    while (running) {
        printf("\n> ");
        fflush(stdout);
        command_read(&command);
        if (command.eof) {
            break;
        }
        running = handle(&hero, &command);
    }
    printf("\nYou made it outside. See you.\n");
}

void game_run() {
    Command command;
    while (1) {
        printf("=== DUNGEON ===\n");
        printf("  new  - new game\n");
        printf("  quit - exit game\n");
        printf("\n> ");
        fflush(stdout);

        command_read(&command);
        if (command.eof) {
            break;
        }
        const char *name = command_name(&command);

        if (command.count == 0) {
            continue;
        }
        if (strcmp(name, "new") == 0) {
            play();
        } else if (strcmp(name, "quit") == 0) {
            break;
        } else {
            printf("Unknown menu item: %s\n\n", name);
        }
    }
}
