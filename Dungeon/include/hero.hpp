#pragma once

#include <string>
#include <vector>

class Map;
class MonsterList;

// Практика 9: герой — класс. Поля: координаты, здоровье, сила удара,
// золото, этаж, состояния и сумка. Методы: шаг, урон, предметы, состояния,
// вывод состояния. На практике 8 всё это лежало в структуре Hero, а функции
// hero_move, hero_damage получали её адрес первым параметром.
class Hero {
public:
    enum class Direction { north, south, east, west };
    enum class Item { potion, antidote, torch };

    static constexpr int GOLD_PER_PILE = 10;
    static constexpr int POTION_HEAL = 10;
    // poisoned, blinded, invisible, petrified, burning, asleep
    static constexpr int STATE_COUNT = 6;

    // Начальное состояние: клетка (x, y), полное здоровье, четыре предмета
    // в сумке. Конструкторов пока нет — начальное состояние задаёт этот метод.
    void reset(int x, int y);

    // Переставляет героя в другую клетку, например на старт нового уровня.
    // Здоровье, золото и сумка остаются.
    void place_at(int x, int y);

    int x() const { return x_; }
    int y() const { return y_; }
    int hp() const { return hp_; }
    int hp_max() const { return hp_max_; }
    int attack() const { return attack_; }
    unsigned int gold() const { return gold_; }
    bool alive() const { return hp_ > 0; }

    // Клетка, куда ведёт шаг в направлении direction. Нужна и для шага,
    // и для удара по соседней клетке.
    void target(Direction direction, int& out_x, int& out_y) const;

    // Шаг: герой спрашивает карту, можно ли туда, и сам меняет свои
    // координаты. Золото под ногами подбирает и убирает с карты.
    // Монстров не проверяет — это делает игра до вызова. Возвращает true,
    // если шаг сделан.
    bool move(Direction direction, Map& map);

    // Урон с насыщением: ниже нуля здоровье не опускается.
    void take_damage(int amount);

    // Сумка. Предметы нумеруются с единицы, как их видит игрок.
    int item_count() const { return static_cast<int>(items_.size()); }
    const char* item_name(int number) const;
    // Использует предмет и убирает его из сумки. Возвращает false, если
    // предмета с таким номером нет.
    bool use_item(int number);

    // Состояния — биты одного числа, как на практике 2. number — номер бита.
    static int state_number(const std::string& word);   // -1, если такого нет
    static const char* state_name(int number);
    bool has_state(int number) const;
    void set_state(int number, bool on);
    void toggle_state(int number);

    void print_status() const;
    void print_inventory() const;

    // Показывает карту с монстрами и собой на ней.
    void look(const Map& map, const MonsterList& monsters) const;

private:
    // Начальные значения прямо в объявлении: у нового героя поля
    // не содержат мусора, даже если reset забыли вызвать.
    int x_ = 1;
    int y_ = 1;
    int hp_ = 30;
    int hp_max_ = 30;
    unsigned char attack_ = 5;   // небольшое неотрицательное число, как в версии на C
    unsigned int gold_ = 0;
    int depth_ = 1;              // номер этажа
    unsigned int states_ = 0;    // набор битов состояний
    // Сумка — вектор: предметы удаляются из середины, и сдвиг хвоста
    // делает erase. На практике 2 это был массив с ручным сдвигом.
    std::vector<Item> items_;
};
