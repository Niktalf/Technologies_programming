#include "monster.hpp"

#include <cstdlib>
#include <stdexcept>
#include <utility>

#include "game.hpp"

int Monster::created_count_ = 0;

// Первой строится часть Creature, потом поле attack_. Если здоровье
// неверное, исключение вылетит из конструктора Creature, и до счётчика
// дело не дойдёт.
Monster::Monster(int x, int y, int hp_max, int attack, MoveRule rule)
    : Creature(x, y, hp_max), attack_(attack), native_rule_(rule), rule_(std::move(rule)) {
    if (attack <= 0) {
        throw std::invalid_argument("monster attack must be positive");
    }
    if (!native_rule_) {
        throw std::invalid_argument("a monster needs a move rule");
    }
    ++created_count_;
}

void Monster::set_rule(MoveRule rule) {
    // Пустое правило не принимается: инвариант «у монстра есть правило».
    if (rule) {
        rule_ = std::move(rule);
    }
}

void Monster::act(Game& game) {
    if (next_to_hero(game)) {
        game.hit_hero(*this);
        return;
    }
    // Вызов правила — вызов функции, которая хранится в поле. Что именно
    // выполнится, решилось тогда, когда правило положили в поле.
    Step step_to = rule_(*this, game);
    if (step_to != Step::stay) {
        step(game, step_dx(step_to), step_dy(step_to));   // занято — стоит на месте
    }
}

bool Monster::next_to_hero(const Game& game) const {
    int dx = std::abs(game.hero().x() - x());
    int dy = std::abs(game.hero().y() - y());
    return dx + dy == 1;
}

bool Monster::step(const Game& game, int dx, int dy) {
    int nx = x() + dx;
    int ny = y() + dy;
    if (!game.free_cell(nx, ny)) {
        return false;
    }
    move_to(nx, ny);
    return true;
}

Goblin::Goblin(int x, int y)
    : Monster(x, y, HP, ATTACK, chase(SIGHT)) {
}

Bat::Bat(int x, int y)
    : Monster(x, y, HP, ATTACK, wander()) {
}

Skeleton::Skeleton(int x, int y)
    : Monster(x, y, HP, ATTACK, stand_still()) {
}

std::unique_ptr<Monster> make_monster(char symbol, int x, int y) {
    // std::make_unique<Goblin> создаёт гоблина в куче и возвращает
    // std::unique_ptr<Goblin>; при возврате он сам превращается
    // в std::unique_ptr<Monster> — указатель на наследника годится
    // как указатель на базу.
    if (symbol == 'g' || symbol == 'M') {
        return std::make_unique<Goblin>(x, y);
    }
    if (symbol == 'b') {
        return std::make_unique<Bat>(x, y);
    }
    if (symbol == 's') {
        return std::make_unique<Skeleton>(x, y);
    }
    return nullptr;
}

char random_monster_symbol() {
    static const char KINDS[3] = { 'g', 'b', 's' };
    return KINDS[std::rand() % 3];
}
