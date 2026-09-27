#include <stdio.h>

#include "hero.h"

void hero_init(Hero *hero)
{
    if (hero == NULL) {
        return;
    }
    hero->hp = 20;
    hero->hp_max = 20;
    hero->attack = 5;
    hero->defense = 2;
    hero->gold = 0;
    hero->states = 0;
}

void hero_take_damage(Hero *hero, const int amount)
{
    if (hero == NULL || amount <= 0) {
        return;
    }

    int reduced = amount - hero->defense;
    if (reduced < 1) {
        reduced = 1;
    }
    hero->hp -= reduced;
    if (hero->hp < 0) {
        hero->hp = 0;
    }
}

void hero_heal(Hero *hero, const int amount)
{
    if (hero == NULL || amount <= 0) {
        return;
    }
    hero->hp += amount;
    if (hero->hp > hero->hp_max) {
        hero->hp = hero->hp_max;
    }
}

int hero_is_alive(const Hero *hero)
{
    return hero != NULL && hero->hp > 0;
}

void hero_state_set(Hero *hero, const uint8_t state)
{
    if (hero != NULL) {
        hero->states |= state;
    }
}

void hero_state_clear(Hero *hero, const uint8_t state)
{
    if (hero != NULL) {
        hero->states &= (uint8_t)~state;
    }
}

int hero_state_has(const Hero *hero, const uint8_t state)
{
    return hero != NULL && (hero->states & state) != 0;
}

void hero_state_toggle(Hero *hero, const uint8_t state)
{
    if (hero != NULL) {
        hero->states ^= state;
    }
}

const char *hero_state_name(const uint8_t state)
{
    switch (state) {
    case STATE_POISONED:  return "poisoned";
    case STATE_INVISIBLE: return "invisible";
    case STATE_IN_FIGHT:  return "in combat";
    case STATE_BLESSED:   return "blessed";
    default:              return "unknown state";
    }
}

void hero_print_status(const Hero *hero)
{
    static const uint8_t ALL_STATES[] = {
        STATE_POISONED, STATE_INVISIBLE, STATE_IN_FIGHT, STATE_BLESSED
    };
    const size_t state_count = sizeof ALL_STATES / sizeof ALL_STATES[0];
    size_t i;
    int printed = 0;

    if (hero == NULL) {
        return;
    }
    printf("Health: %d / %d\n", hero->hp, hero->hp_max);
    printf("Attack: %d, Defense: %d\n", hero->attack, hero->defense);
    printf("Gold: %u\n", hero->gold);
    printf("Statuses: ");

    for (i = 0; i < state_count; ++i) {
        if (hero_state_has(hero, ALL_STATES[i])) {
            printf("%s%s", printed ? ", " : "", hero_state_name(ALL_STATES[i]));
            printed = 1;
        }
    }
    if (!printed) {
        printf("no");
    }
    printf("\n");
    printf("State mask: 0x%02X\n", (unsigned)hero->states);
}
