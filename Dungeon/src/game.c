#include <stdio.h>

#include "command.h"
#include "game.h"
#include "hero.h"
#include "map.h"

#define TRAP_DAMAGE 7
#define REST_HEAL   4

static void print_help()
{
    printf("Available commands:\n");
    printf("  look      - look around\n");
    printf("  north, south, east, west (or n, s, e, w) - step\n");
    printf("  status    - hero characteristics and states\n");
    printf("  hit       - take damage from a trap\n");
    printf("  rest      - rest and restore health\n");
    printf("  poison    - toggle the poisoned state\n");
    printf("  help      - this list\n");
    printf("  quit      - exit the game\n");
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

static void play(void)
{
    Hero hero;
    int running = 1;

    hero_init(&hero);
    map_init();

    printf("You are going down into the dungeon.\n\n");
    map_draw(hero.x, hero.y);

    while (running) {
        printf("\n> ");
        fflush(stdout);
        const Command command = command_read();

        switch (command) {
        case CMD_LOOK:
            map_draw(hero.x, hero.y);
            break;
        case CMD_NORTH:
            if (try_move(&hero, 0, -1)) {
                map_draw(hero.x, hero.y);
            }
            break;
        case CMD_SOUTH:
            if (try_move(&hero, 0, 1)) {
                map_draw(hero.x, hero.y);
            }
            break;
        case CMD_EAST:
            if (try_move(&hero, 1, 0)) {
                map_draw(hero.x, hero.y);
            }
            break;
        case CMD_WEST:
            if (try_move(&hero, -1, 0)) {
                map_draw(hero.x, hero.y);
            }
            break;
        case CMD_STATUS:
            hero_print_status(&hero);
            break;
        case CMD_HIT:
            hero_take_damage(&hero, TRAP_DAMAGE);
            hero_state_set(&hero, STATE_IN_FIGHT);
            printf("The trap hits you. Health: %d / %d\n", hero.hp, hero.hp_max);
            if (!hero_is_alive(&hero)) {
                printf("You died in the dungeon.\n");
                running = 0;
            }
            break;
        case CMD_REST:
            hero_heal(&hero, REST_HEAL);
            hero_state_clear(&hero, STATE_IN_FIGHT);
            printf("You are resting. Health: %d / %d\n", hero.hp, hero.hp_max);
            break;
        case CMD_POISON:
            hero_state_toggle(&hero, STATE_POISONED);
            printf("Status \"%s\": %s\n",
                   hero_state_name(STATE_POISONED),
                   hero_state_has(&hero, STATE_POISONED) ? "enabled" : "disabled");
            break;
        case CMD_HELP:
            print_help();
            break;
        case CMD_EMPTY:
            break;
        case CMD_QUIT:
        case CMD_EOF:
            running = 0;
            break;
        case CMD_UNKNOWN:
        default:
            printf("Unknown command: %s\n", command_last_word());
            printf("Type help to see the list of commands.\n");
            break;
        }
    }
    printf("\nYou made it outside. See you.\n");
}

void game_run(void)
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
