#include "monster.hpp"

#include <cstdlib>
#include <stdexcept>

#include "game.hpp"

int Monster::created_count_ = 0;

// Первой строится часть Creature, потом поле attack_. Если здоровье
// неверное, исключение вылетит из конструктора Creature, и до счётчика
// дело не дойдёт.
Monster::Monster(int x, int y, int hp_max, int attack)
    : Creature(x, y, hp_max), attack_(attack) {
    if (attack <= 0) {
        throw std::invalid_argument("monster attack must be positive");
    }
    ++created_count_;
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
    : Monster(x, y, HP, ATTACK) {
}

void Goblin::act(Game& game) {
    if (next_to_hero(game)) {
        game.hit_hero(*this);
        return;
    }
    int dx = game.hero().x() - x();
    int dy = game.hero().y() - y();
    if (std::abs(dx) + std::abs(dy) > SIGHT) {
        return;   // героя не видно — стоит
    }
    // Шаг по той оси, по которой до героя дальше. Не вышло — по другой.
    int sx = dx > 0 ? 1 : (dx < 0 ? -1 : 0);
    int sy = dy > 0 ? 1 : (dy < 0 ? -1 : 0);
    bool horizontal_first = std::abs(dx) >= std::abs(dy);
    int first_dx = horizontal_first ? sx : 0;
    int first_dy = horizontal_first ? 0 : sy;
    int second_dx = horizontal_first ? 0 : sx;
    int second_dy = horizontal_first ? sy : 0;
    bool moved = (first_dx != 0 || first_dy != 0) && step(game, first_dx, first_dy);
    if (!moved && (second_dx != 0 || second_dy != 0)) {
        step(game, second_dx, second_dy);
    }
}

Bat::Bat(int x, int y)
    : Monster(x, y, HP, ATTACK) {
}

void Bat::act(Game& game) {
    static const int DX[4] = { 0, 0, 1, -1 };
    static const int DY[4] = { -1, 1, 0, 0 };
    int direction = std::rand() % 4;
    int nx = x() + DX[direction];
    int ny = y() + DY[direction];
    if (nx == game.hero().x() && ny == game.hero().y()) {
        game.hit_hero(*this);
        return;
    }
    step(game, DX[direction], DY[direction]);   // занято — мышь остаётся на месте
}

Skeleton::Skeleton(int x, int y)
    : Monster(x, y, HP, ATTACK) {
}

void Skeleton::act(Game& game) {
    if (next_to_hero(game)) {
        game.hit_hero(*this);
    }
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
