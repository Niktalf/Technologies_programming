#include <stdio.h>
#include <string.h>

#include "game.h"
#include "hero.h"

/* Буфер под слово команды: 15 символов и завершающий ноль. */
#define COMMAND_SIZE 16

/* Названия состояний в порядке битов: элемент номер i описывает бит номер i.
   Одно и то же слово служит и аргументом команды, и текстом в выводе. */
static const char *const STATE_NAMES[STATE_COUNT] = {
    "poisoned", "blinded", "invisible", "petrified", "burning", "asleep"
};

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
    printf("Commands: north, south, east, west, look, status, hit <N>,\n");
    printf("         on <state>, off <state>, toggle <state>, quit\n");
    printf("States: poisoned, blinded, invisible, petrified, burning, asleep\n");
}

/* Выбрасывает остаток строки ввода. Нужна после неудачного чтения числа:
   scanf оставляет нечисловые символы во входном потоке, и без этого
   они были бы прочитаны как следующая команда. */
static void skip_rest_of_line(void) {
    int c = getchar();
    while (c != '\n' && c != EOF) {
        c = getchar();
    }
}

void game_run(void) {
    /* Здоровье знаковое, хотя отрицательным быть не должно. Урон вычитается,
       и промежуточный результат может уйти ниже нуля — у беззнакового типа
       он превратился бы в огромное положительное число, и проверка
       «меньше нуля» никогда бы не сработала. Диапазона int с запасом
       хватает на любое здоровье. */
    int hp = 30;
    int hp_max = 30;
    /* Сила атаки — небольшое неотрицательное число. unsigned char хватает
       до 255; заодно тип говорит читателю, что отрицательной атака не бывает. */
    unsigned char attack = 5;
    /* Золото только копится и отрицательным не бывает. unsigned int
       вмещает больше четырёх миллиардов — с запасом. */
    unsigned int gold = 0;
    /* Глубина этажа — номер от 1 до нескольких десятков. Обычный int:
       глубины будем сравнивать и вычитать, и знаковый тип здесь надёжнее. */
    int depth = 1;
    /* Шесть состояний в одной переменной, по биту на состояние. */
    unsigned int states = 0;

    printf("Welcome to the dungeon!\n\n");
    print_picture();
    printf("\n");
    print_commands();

    int turns = 0;
    int running = 1;
    while (running) {
        printf("\nTurn %d > ", turns);
        fflush(stdout);

        char command[COMMAND_SIZE];
        if (scanf("%15s", command) != 1) {
            running = 0;
        } else if (strcmp(command, "quit") == 0) {
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
            print_picture();
        } else if (strcmp(command, "status") == 0) {
            printf("Health: %d / %d\n", hp, hp_max);
            /* Сначала неправильно: hp / hp_max — деление целых, и при
               hp < hp_max результат 0. Умножать на 100 уже нечего. */
            printf("Health percent without cast: %d%%\n", hp / hp_max * 100);
            /* Правильно: приводим к double до деления, а не после. */
            printf("Health percent with cast:    %.1f%%\n",
                   (double)hp / (double)hp_max * 100.0);
            printf("Attack: %u, gold: %u, floor: %d\n", (unsigned)attack, gold, depth);
            printf("States (bits 0x%02X):", states);
            int any = 0;
            for (int i = 0; i < STATE_COUNT; ++i) {
                /* Проверка бита: результат «И» равен самой маске, а не единице,
                   поэтому сравниваем с нулём. */
                if ((states & (1u << i)) != 0) {
                    printf(" %s", STATE_NAMES[i]);
                    any = 1;
                }
            }
            printf("%s\n", any ? "" : " none");
        } else if (strcmp(command, "hit") == 0) {
            /* Ширина 9 в формате: число из девяти цифр всегда помещается в int,
               а длиннее scanf и не прочитает. */
            int damage = 0;
            int ok = scanf("%9d", &damage) == 1 && damage > 0;

            /* Остаток строки выбрасываем в любом случае: в «hit 12345678901»
               после девяти цифр останутся ещё две, и они стали бы командой. */
            skip_rest_of_line();
            if (!ok) {
                printf("hit needs a positive number, e.g. hit 7\n");
            } else {
                hp = hp - damage;
                if (hp < 0) {
                    hp = 0; /* ниже нуля здоровье не опускается */
                }
                printf("The hero takes %d damage. Health: %d / %d\n", damage, hp, hp_max);
                if (hp == 0) {
                    printf("The hero is dead.\n");
                    running = 0;
                }
            }
        } else if (strcmp(command, "on") == 0 || strcmp(command, "off") == 0
                   || strcmp(command, "toggle") == 0) {
            char word[COMMAND_SIZE];
            int found = -1;
            if (scanf("%15s", word) == 1) {
                for (int i = 0; i < STATE_COUNT; ++i) {
                    if (strcmp(word, STATE_NAMES[i]) == 0) {
                        found = i;
                    }
                }
            }
            if (found < 0) {
                printf("No such state. Known: poisoned, blinded, invisible,\n");
                printf("petrified, burning, asleep\n");
            } else {
                unsigned int mask = 1u << found;
                if (strcmp(command, "on") == 0) {
                    states = states | mask; /* установить: «ИЛИ» с маской */
                } else if (strcmp(command, "off") == 0) {
                    states = states & ~mask; /* снять: «И» с инверсией маски */
                } else {
                    states = states ^ mask; /* переключить: исключающее «ИЛИ» */
                }
                printf("Hero is %s: %s\n", STATE_NAMES[found],
                       (states & mask) != 0 ? "yes" : "no");
            }
        } else {
            printf("Unknown command: %s\n", command);
            print_commands();
        }
    }

    printf("\nGame over. Turns made: %d\n", turns);
}
