#ifndef HERO_H
#define HERO_H

#include <stdint.h>

#define STATE_POISONED  (1u << 0)
#define STATE_INVISIBLE (1u << 1)
#define STATE_IN_FIGHT  (1u << 2)
#define STATE_BLESSED   (1u << 3)

#define HERO_HP_MAX_LIMIT 999

typedef struct {
    int      x;
    int      y;

    int      hp;
    int      hp_max;
    int      attack;
    int      defense;

    uint32_t gold;
    uint8_t  states;
} Hero;

void hero_init(Hero *hero);

void hero_add_gold(Hero *hero, unsigned int amount);
void hero_take_damage(Hero *hero, int amount);
void hero_heal(Hero *hero, int amount);
int hero_is_alive(const Hero *hero);

void hero_state_set(Hero *hero, uint8_t state);
void hero_state_clear(Hero *hero, uint8_t state);
int  hero_state_has(const Hero *hero, uint8_t state);
void hero_state_toggle(Hero *hero, uint8_t state);

const char *hero_state_name(uint8_t state);
void hero_print_status(const Hero *hero);

#endif // HERO_H
