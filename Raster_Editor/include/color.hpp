#pragma once

#include <cstdint>

// Практика 9: цвет — класс с тремя каналами.
//
// В версии на C цвет был упакованным числом Pixel, а операции над ним —
// свободными функциями: pixel_adjust_brightness(pixel, delta). Здесь данные
// и операции вместе: color.brighten(40). Поля закрыты, и поменять канал
// в обход методов нельзя.
class Color {
public:
    static constexpr int CHANNEL_MAX = 255;

    // Цвет по трём каналам. Конструкторов пока нет (они — на практике 10),
    // поэтому цвет создаёт статический метод: он принадлежит классу,
    // а не объекту, и вызывается через имя класса — Color::from_rgb(...).
    static Color from_rgb(std::uint8_t red, std::uint8_t green, std::uint8_t blue);

    // Цвет из числа 0xRRGGBB и обратно — запись, привычная по версии на C.
    static Color from_packed(std::uint32_t packed);
    std::uint32_t packed() const;

    // Значение в пределах канала: меньше нуля — 0, больше 255 — 255.
    static std::uint8_t clamp(int value);

    std::uint8_t red() const { return red_; }
    std::uint8_t green() const { return green_; }
    std::uint8_t blue() const { return blue_; }

    // Те же ли каналы, что у other.
    bool same_as(Color other) const;

    // Осветляет и затемняет на amount с насыщением: канал не выходит
    // за 0 и 255. Возвращают ссылку на сам цвет, поэтому вызовы можно
    // выстраивать цепочкой: color.brighten(40).darken(10).
    Color& brighten(int amount);
    Color& darken(int amount);

    // Яркость по взвешенной формуле, серый цвет той же яркости
    // и противоположный цвет. const после списка параметров: метод
    // не меняет объект, а возвращает новый цвет.
    std::uint8_t luminance() const;
    Color to_gray() const;
    Color inverted() const;

private:
    std::uint8_t red_ = 0;
    std::uint8_t green_ = 0;
    std::uint8_t blue_ = 0;
};
