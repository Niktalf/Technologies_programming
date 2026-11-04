#pragma once

#include <string>
#include <vector>

class Map;

// Герой. Поля: координаты, здоровье, сила удара, золото, этаж, состояния
// и сумка. Методы: шаг, урон, предметы, состояния, вывод состояния.
//
// Инвариант:
//   * hp_max() > 0 и 0 <= hp() <= hp_max();
//   * в states_ включены только биты с номерами от 0 до STATE_COUNT - 1;
//   * золото меняется только подбором кучки, по GOLD_PER_PILE.
// Конструктор устанавливает инвариант, а каждый изменяющий метод его
// сохраняет: урон не опускает здоровье ниже нуля, зелье не поднимает выше
// максимума, неверный номер состояния отвергается.
//
// Что герой стоит внутри карты и не в стене, — инвариант не героя,
// а игры: герой не знает, на какой он карте. Его гарантируют move,
// который спрашивает карту, и place_at, который тоже её спрашивает.
//
// Практика 10: два конструктора. Герой не владеет ресурсами вручную: сумка —
// std::vector, и он освобождает себя сам. Поэтому ни деструктор, ни
// копирование, ни перемещение герою писать не нужно — подойдут созданные
// компилятором. Это правило нуля: если все поля сами умеют копироваться
// и освобождаться, особые методы не нужны.
class Hero {
public:
    enum class Direction { north, south, east, west };
    enum class Item { potion, antidote, torch };

    static constexpr int GOLD_PER_PILE = 10;
    static constexpr int POTION_HEAL = 10;
    // poisoned, blinded, invisible, petrified, burning, asleep
    static constexpr int STATE_COUNT = 6;

    static constexpr int START_HP = 30;

    // Конструктор по умолчанию: клетка (1, 1), полное здоровье, четыре
    // предмета в сумке.
    Hero();

    // Герой в заданной клетке с заданным здоровьем и тем же набором
    // предметов. Здоровье hp <= 0 нарушило бы инвариант — тогда
    // std::invalid_argument, и героя нет.
    Hero(int x, int y, int hp = START_HP);

    // Переставляет героя в клетку (x, y) карты map, например на старт нового
    // уровня. Здоровье, золото и сумка остаются. Если клетка вне карты или
    // это стена, герой остаётся на месте и метод возвращает false.
    bool place_at(const Map& map, int x, int y);

    // Геттеры — только те, что нужны снаружи: игре и отрисовке. Геттера
    // золота нет: золото снаружи никто не читает, его показывает сам герой
    // в print_status. Сеттеров нет вовсе: здоровье меняют урон и зелье,
    // координаты — шаг, и каждый из этих методов сохраняет инвариант.
    int x() const { return x_; }
    int y() const { return y_; }
    int hp() const { return hp_; }
    int hp_max() const { return hp_max_; }
    int attack() const { return attack_; }
    bool alive() const { return hp_ > 0; }

    // Клетка, куда ведёт шаг в направлении direction. Нужна и для шага,
    // и для удара по соседней клетке.
    void target(Direction direction, int& out_x, int& out_y) const;

    // Шаг: герой спрашивает карту, можно ли туда, и сам меняет свои
    // координаты. Золото под ногами подбирает и убирает с карты.
    // Монстров не проверяет — это делает игра до вызова. Возвращает true,
    // если шаг сделан.
    bool move(Direction direction, Map& map);

    // Урон с насыщением: ниже нуля здоровье не опускается. Отрицательный
    // урон ничего не делает: иначе им можно было бы лечить сверх максимума.
    void take_damage(int amount);

    // Сумка. Предметы нумеруются с единицы, как их видит игрок.
    int item_count() const { return static_cast<int>(items_.size()); }
    const char* item_name(int number) const;
    // Использует предмет и убирает его из сумки. Возвращает false, если
    // предмета с таким номером нет.
    bool use_item(int number);

    // Состояния — биты одного числа, как на практике 2. number — номер бита
    // от 0 до STATE_COUNT - 1; другой номер — std::out_of_range.
    static int state_number(const std::string& word);   // -1, если такого нет
    static const char* state_name(int number);
    bool has_state(int number) const;
    void set_state(int number, bool on);
    void toggle_state(int number);

    void print_status() const;
    void print_inventory() const;

private:
    // Проверка номера состояния — одна на четыре метода, поэтому вынесена
    // в закрытый метод.
    static void check_state(int number);

    // Значения полям даёт конструктор: объект без конструктора теперь
    // создать нельзя. Остальные поля одинаковы у всех новых героев
    // и получают значения прямо в объявлении.
    int x_;
    int y_;
    int hp_;
    int hp_max_;
    unsigned char attack_ = 5;   // небольшое неотрицательное число, как в версии на C
    unsigned int gold_ = 0;
    int depth_ = 1;              // номер этажа
    unsigned int states_ = 0;    // набор битов состояний
    // Сумка — вектор: предметы удаляются из середины, и сдвиг хвоста
    // делает erase. На практике 2 это был массив с ручным сдвигом.
    std::vector<Item> items_;
};
