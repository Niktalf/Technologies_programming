#include <stdio.h>

#include "command.h"
#include "game.h"
#include "map.h"

#define HERO_START_X 2
#define HERO_START_Y 2

static void print_help()
{
    printf("Available commands:\n");
    printf("  look - look around\n");
    printf("  help - this list\n");
    printf("  quit - exit the game\n");
}

static void play()
{
    int running = 1;

    printf("You are going down into the dungeon.\n\n");
    map_draw(HERO_START_X, HERO_START_Y);

    while (running) {
        printf("\n> ");
        fflush(stdout);

        const Command command = command_read();

        switch (command) {
        case CMD_LOOK:
            map_draw(HERO_START_X, HERO_START_Y);
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
