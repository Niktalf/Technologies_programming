#include "move_rule.hpp"

#include <cstdlib>

#include "game.hpp"
#include "monster.hpp"

namespace {

// Шаг от (x, y) к (tx, ty): по оси, по которой до цели дальше.
// Общая часть «идти к» и «уходить от» — чтобы лямбды не повторяли друг
// друга.
Step towards(int x, int y, int tx, int ty) {
    int dx = tx - x;
    int dy = ty - y;
    if (dx == 0 && dy == 0) {
        return Step::stay;
    }
    if (std::abs(dx) >= std::abs(dy)) {
        return dx > 0 ? Step::east : Step::west;
    }
    return dy > 0 ? Step::south : Step::north;
}

Step opposite(Step step) {
    switch (step) {
    case Step::north:
        return Step::south;
    case Step::south:
        return Step::north;
    case Step::east:
        return Step::west;
    case Step::west:
        return Step::east;
    case Step::stay:
        return Step::stay;
    }
    return Step::stay;
}

Step random_step() {
    return static_cast<Step>(1 + std::rand() % 4);   // north, south, east или west
}

// Расстояние от монстра до героя в шагах по сетке.
int distance_to_hero(const Monster& self, const Game& game) {
    return std::abs(game.hero().x() - self.x()) + std::abs(game.hero().y() - self.y());
}

} // namespace

int step_dx(Step step) {
    return step == Step::east ? 1 : (step == Step::west ? -1 : 0);
}

int step_dy(Step step) {
    return step == Step::south ? 1 : (step == Step::north ? -1 : 0);
}

// Лямбда с захватом: [sight] копирует параметр внутрь лямбды. Функция
// chase закончится, её параметр исчезнет, а копия в лямбде останется —
// столько, сколько живёт правило. Захвати мы sight по ссылке, [&sight],
// правило ссылалось бы на переменную, которой уже нет.
MoveRule chase(int sight) {
    return [sight](const Monster& self, const Game& game) {
        if (distance_to_hero(self, game) > sight) {
            return Step::stay;
        }
        return towards(self.x(), self.y(), game.hero().x(), game.hero().y());
    };
}

MoveRule flee(int sight) {
    return [sight](const Monster& self, const Game& game) {
        if (distance_to_hero(self, game) > sight) {
            return Step::stay;
        }
        return opposite(towards(self.x(), self.y(), game.hero().x(), game.hero().y()));
    };
}

MoveRule stand_still() {
    // Лямбда без захвата — по сути обычная функция. Поэтому её можно
    // присвоить простому указателю на функцию — так и сделано здесь.
    // Лямбду из chase так присвоить нельзя: ошибка компиляции.
    Step (*plain)(const Monster&, const Game&) = [](const Monster&, const Game&) {
        return Step::stay;
    };
    return plain;   // std::function принимает и указатель на функцию
}

MoveRule wander() {
    return [](const Monster&, const Game&) { return random_step(); };
}

// base захвачен по значению: новое правило хранит свою копию старого.
// Захват по ссылке был бы ошибкой — base живёт только до конца drunk.
MoveRule drunk(MoveRule base, int chance) {
    return [base, chance](const Monster& self, const Game& game) {
        if (std::rand() % chance == 0) {
            return random_step();
        }
        return base(self, game);
    };
}

MoveRule rule_by_name(const std::string& name) {
    if (name == "chase") {
        return chase(8);
    }
    if (name == "flee") {
        return flee(8);
    }
    if (name == "stand") {
        return stand_still();
    }
    if (name == "wander") {
        return wander();
    }
    return MoveRule();   // пустое правило
}
