#include "hero.hpp"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <stdexcept>

#include "map.hpp"

namespace {

// Названия состояний в порядке битов: элемент номер i описывает бит номер i.
const char* const STATE_NAMES[Hero::STATE_COUNT] = {
    "poisoned", "blinded", "invisible", "petrified", "burning", "asleep"
};

// Номера битов: яд и слепоту снимают противоядие и факел, яд и огонь
// отнимают здоровье в конце каждого хода.
constexpr int STATE_POISONED = 0;
constexpr int STATE_BLINDED = 1;
constexpr int STATE_BURNING = 4;

} // namespace

// Конструктор по умолчанию передаёт работу второму конструктору:
// так начальные значения записаны в одном месте.
Hero::Hero()
    : Hero(1, 1) {
}

// Первым в списке инициализации стоит конструктор базового класса: сначала
// строится часть Creature — координаты и здоровье, — потом поля героя.
// Здоровье проверяет конструктор Creature: проверка в одном месте на всех
// существ.
Hero::Hero(int x, int y, int hp)
    : Creature(x, y, hp),
      items_{ Item::potion, Item::antidote, Item::torch, Item::potion } {
}

// Яд и огонь — то, что происходит с героем само, в конце каждого хода.
void Hero::act(Game&) {
    if (has_state(STATE_POISONED)) {
        take_damage(POISON_DAMAGE);
        std::cout << "The poison takes " << POISON_DAMAGE << " health. Health: " << hp()
                  << " / " << hp_max() << '\n';
    }
    if (has_state(STATE_BURNING)) {
        take_damage(FIRE_DAMAGE);
        std::cout << "The fire takes " << FIRE_DAMAGE << " health. Health: " << hp()
                  << " / " << hp_max() << '\n';
    }
}

bool Hero::place_at(const Map& map, int x, int y) {
    if (!map.walkable(x, y)) {   // walkable сам отвечает false за пределами карты
        return false;
    }
    move_to(x, y);   // поля координат закрыты в Creature, меняет их защищённый метод
    return true;
}

void Hero::target(Direction direction, int& out_x, int& out_y) const {
    out_x = x();
    out_y = y();
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
    move_to(nx, ny);
    if (map.tile(nx, ny) == Map::GOLD) {
        gold_ += GOLD_PER_PILE;
        map.set_tile(nx, ny, Map::FLOOR);
        std::cout << "The hero picks up " << GOLD_PER_PILE << " gold. Total: " << gold_ << '\n';
    }
    if (map.tile(nx, ny) == Map::EXIT) {
        std::cout << "Here is the exit to the next floor. Going down comes later.\n";
    }
    return true;
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
        heal(POTION_HEAL);   // насыщение до максимума — забота Creature
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
    // std::find идёт по массиву имён и сравнивает каждое с word: std::string
    // сравнивается с const char* по содержимому.
    const char* const* end = STATE_NAMES + STATE_COUNT;
    const char* const* found = std::find(STATE_NAMES, end, word);
    return found == end ? -1 : static_cast<int>(found - STATE_NAMES);
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
    double percent = static_cast<double>(hp()) / hp_max() * 100.0;
    std::cout << "Health: " << hp() << " / " << hp_max() << " (" << std::fixed
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
