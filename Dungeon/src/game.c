#include <stdio.h>

#include "command.h"
#include "game.h"
#include "hero.h"
#include "map.h"
#include "reach.h"

#define TRAP_DAMAGE 7
#define REST_HEAL   4

static void print_help()
{
    printf("Available commands:\n");
    printf("  look      - watch\n");
    printf("  north, south, east, west (or n, s, e, w) - step\n");
    printf("  status    - characteristics and status of the hero\n");
    printf("  reach     - is the ladder achievable from here\n");
    printf("  hit       - take damage from a trap\n");
    printf("  rest      - rest and restore health\n");
    printf("  poison    - switch the infection state\n");
    printf("  help      - this list\n");
    printf("  quit      - get out of the game\n");
}

static void print_reach(const Hero *hero)
{
    const ReachReport report = reach_check(hero->x, hero->y);

    printf("Ladder %s.\n", report.stairs_reachable ? "attainable" : "unattainable");
    printf("Achievable cells: %d, recursion depth: %d\n",
           report.cells, report.max_depth);
}

static int try_move(Hero *hero, const int dx, const int dy)
{
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

static int handle(Hero *hero, const Command command)
{
    switch (command) {
        case CMD_LOOK:
            map_draw(hero->x, hero->y);
            return 1;
        case CMD_NORTH:
            if (try_move(hero, 0, -1)) {
                map_draw(hero->x, hero->y);
            }
            return 1;
        case CMD_SOUTH:
            if (try_move(hero, 0, 1)) {
                map_draw(hero->x, hero->y);
            }
            return 1;
        case CMD_EAST:
            if (try_move(hero, 1, 0)) {
                map_draw(hero->x, hero->y);
            }
            return 1;
        case CMD_WEST:
            if (try_move(hero, -1, 0)) {
                map_draw(hero->x, hero->y);
            }
            return 1;
        case CMD_REACH:
            print_reach(hero);
            return 1;
        case CMD_STATUS:
            hero_print_status(hero);
            return 1;
        case CMD_HIT:
            hero_take_damage(hero, TRAP_DAMAGE);
            hero_state_set(hero, STATE_IN_FIGHT);
            printf("The trap hits you. Health: %d / %d\n", hero->hp, hero->hp_max);
            if (!hero_is_alive(hero)) {
                printf("You died in the dungeon.\n");
                return 0;
            }
            return 1;
        case CMD_REST:
            hero_heal(hero, REST_HEAL);
            hero_state_clear(hero, STATE_IN_FIGHT);
            printf("You are resting. Health: %d / %d\n", hero->hp, hero->hp_max);
            return 1;
        case CMD_POISON:
            hero_state_toggle(hero, STATE_POISONED);
            printf("Status \"%s\": %s\n",
                   hero_state_name(STATE_POISONED),
                   hero_state_has(hero, STATE_POISONED) ? "enabled" : "disabled");
            return 1;
        case CMD_HELP:
            print_help();
            return 1;
        case CMD_EMPTY:
            return 1;
        case CMD_QUIT:
        case CMD_EOF:
            return 0;
        case CMD_UNKNOWN:
        default:
            printf("Unknown command: %s\n", command_last_word());
            printf("Type help to see the list of commands.\n");
            return 1;
        }
}

static void play()
{
    Hero hero;
    int running = 1;

    hero_init(&hero);
    map_init();

    printf("You're going down to the dungeon.\n\n");
    map_draw(hero.x, hero.y);
    print_reach(&hero);

    while (running) {
        printf("\n> ");
        fflush(stdout);
        running = handle(&hero, command_read());
    }
    printf("\nYou made it outside. See you.\n");
}

void game_run()
{
    int running = 1;
    while (running) {
        printf("=== DUNGEON ===\n");
        printf("  new  - new game\n");
        printf("  quit - exit game\n");
        printf("\n> ");
        fflush(stdout);

        const Command command = command_read();
        switch (command) {
        case CMD_NEW:
            play();
            break;
        case CMD_EMPTY:
            break;
        case CMD_QUIT:
        case CMD_EOF:
            running = 0;
            break;
        default:
            printf("Unknown menu item: %s\n\n", command_last_word());
            break;
        }
    }
}
