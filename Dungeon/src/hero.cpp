#include "hero.hpp"

#include <iomanip>
#include <iostream>

#include "map.hpp"

namespace {

// Названия состояний в порядке битов: элемент номер i описывает бит номер i.
const char* const STATE_NAMES[Hero::STATE_COUNT] = {
    "poisoned", "blinded", "invisible", "petrified", "burning", "asleep"
};

// Номера битов, которые снимают противоядие и факел.
constexpr int STATE_POISONED = 0;
constexpr int STATE_BLINDED = 1;

} // namespace

void Hero::reset(int x, int y) {
    x_ = x;
    y_ = y;
    hp_max_ = 30;
    hp_ = hp_max_;
    attack_ = 5;
    gold_ = 0;
    depth_ = 1;
    states_ = 0;
    items_ = { Item::potion, Item::antidote, Item::torch, Item::potion };
}

void Hero::place_at(int x, int y) {
    x_ = x;
    y_ = y;
}

void Hero::target(Direction direction, int& out_x, int& out_y) const {
    out_x = x_;
    out_y = y_;
    if (direction == Direction::north) {
        --out_y;   // север — к меньшему номеру строки
    } else if (direction == Direction::south) {
        ++out_y;
    } else if (direction == Direction::east) {
        ++out_x;
    } else {
        --out_x;
    }
}

bool Hero::move(Direction direction, Map& map) {
    int nx = 0;
    int ny = 0;
    target(direction, nx, ny);
    if (!map.walkable(nx, ny)) {
        return false;
    }
    x_ = nx;
    y_ = ny;
    if (map.tile(x_, y_) == Map::GOLD) {
        gold_ += GOLD_PER_PILE;
        map.set_tile(x_, y_, Map::FLOOR);
        std::cout << "The hero picks up " << GOLD_PER_PILE << " gold. Total: " << gold_ << '\n';
    }
    if (map.tile(x_, y_) == Map::EXIT) {
        std::cout << "Here is the exit to the next floor. Going down comes later.\n";
    }
    return true;
}

void Hero::take_damage(int amount) {
    hp_ -= amount;
    if (hp_ < 0) {
        hp_ = 0;
    }
}

const char* Hero::item_name(int number) const {
    if (number < 1 || number > item_count()) {
        return "unknown item";
    }
    Item item = items_[static_cast<std::size_t>(number - 1)];
    if (item == Item::potion) {
        return "healing potion";
    }
    if (item == Item::antidote) {
        return "antidote";
    }
    return "torch";
}

bool Hero::use_item(int number) {
    if (number < 1 || number > item_count()) {
        return false;
    }
    auto position = items_.begin() + (number - 1);
    if (*position == Item::potion) {
        hp_ += POTION_HEAL;
        if (hp_ > hp_max_) {
            hp_ = hp_max_;
        }
    } else if (*position == Item::antidote) {
        set_state(STATE_POISONED, false);
    } else {
        set_state(STATE_BLINDED, false);
    }
    // Предмет израсходован: erase сдвигает хвост сумки на его место,
    // и порядок остальных предметов не меняется.
    items_.erase(position);
    return true;
}

int Hero::state_number(const std::string& word) {
    for (int i = 0; i < STATE_COUNT; ++i) {
        if (word == STATE_NAMES[i]) {   // std::string сравнивается по содержимому
            return i;
        }
    }
    return -1;
}

const char* Hero::state_name(int number) {
    return STATE_NAMES[number];
}

bool Hero::has_state(int number) const {
    return (states_ & (1u << number)) != 0;
}

void Hero::set_state(int number, bool on) {
    unsigned int mask = 1u << number;
    states_ = on ? states_ | mask : states_ & ~mask;
}

void Hero::toggle_state(int number) {
    states_ ^= 1u << number;
}

void Hero::print_status() const {
    double percent = static_cast<double>(hp_) / hp_max_ * 100.0;
    std::cout << "Health: " << hp_ << " / " << hp_max_ << " (" << std::fixed
              << std::setprecision(1) << percent << "%)\n";
    std::cout << "Attack: " << static_cast<int>(attack_) << ", gold: " << gold_
              << ", floor: " << depth_ << '\n';
    std::cout << "States:";
    bool any = false;
    for (int i = 0; i < STATE_COUNT; ++i) {
        if (has_state(i)) {
            std::cout << ' ' << STATE_NAMES[i];
            any = true;
        }
    }
    std::cout << (any ? "" : " none") << '\n';
}

void Hero::print_inventory() const {
    if (items_.empty()) {
        std::cout << "The bag is empty.\n";
        return;
    }
    for (int i = 1; i <= item_count(); ++i) {
        std::cout << "  " << i << ". " << item_name(i) << '\n';
    }
}

// Явный this. Вызов hero.look(map, monsters) компилятор превращает в вызов
// обычной функции, которой скрытым первым параметром передан адрес hero —
// это и есть this. Внутри метода x_ означает this->x_: поле того героя,
// через которого метод вызван. В этом и отличие вызова метода от вызова
// функции: в версии на C адрес героя пришлось бы передать явно.
//
// Здесь карте нужно передать самого героя, чтобы она узнала, где его
// нарисовать. Изнутри метода «этот объект» по-другому не назвать: this — его адрес.
void Hero::look(const Map& map, const MonsterList& monsters) const {
    map.draw(this, &monsters);
}
