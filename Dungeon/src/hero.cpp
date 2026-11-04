#include "hero.hpp"

#include <iomanip>
#include <iostream>
#include <stdexcept>

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

// Конструктор по умолчанию передаёт работу второму конструктору:
// так начальные значения записаны в одном месте.
Hero::Hero()
    : Hero(1, 1) {
}

// Список инициализации: поля получают значения до входа в тело
// конструктора, в порядке объявления в классе.
Hero::Hero(int x, int y, int hp)
    : x_(x), y_(y), hp_(hp), hp_max_(hp),
      items_{ Item::potion, Item::antidote, Item::torch, Item::potion } {
    // Проверка в конструкторе, а не в вызывающем коде: мест, где создаётся
    // герой, может быть сколько угодно, а конструктор один.
    if (hp <= 0) {
        throw std::invalid_argument("hero health must be positive");
    }
}

bool Hero::place_at(const Map& map, int x, int y) {
    if (!map.walkable(x, y)) {   // walkable сам отвечает false за пределами карты
        return false;
    }
    x_ = x;
    y_ = y;
    return true;
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
    if (amount <= 0) {
        return;
    }
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

void Hero::check_state(int number) {
    if (number < 0 || number >= STATE_COUNT) {
        throw std::out_of_range("no such hero state");
    }
}

// Раньше неверный номер читал чужую память за концом массива имён,
// а сдвиг 1u << 40 — неопределённое поведение. Теперь он отвергается.
const char* Hero::state_name(int number) {
    check_state(number);
    return STATE_NAMES[number];
}

bool Hero::has_state(int number) const {
    check_state(number);
    return (states_ & (1u << number)) != 0;
}

void Hero::set_state(int number, bool on) {
    check_state(number);
    unsigned int mask = 1u << number;
    states_ = on ? states_ | mask : states_ & ~mask;
}

void Hero::toggle_state(int number) {
    check_state(number);
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
