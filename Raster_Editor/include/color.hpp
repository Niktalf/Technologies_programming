#pragma once

#include <cstdint>

// Цвет: три канала.
//
// Инвариант: каждый канал в пределах 0..CHANNEL_MAX. Его держит сам тип
// поля — std::uint8_t другого значения не вмещает. Но тип не насыщает,
// а обрезает: 300, записанное в uint8_t, становится 44. Поэтому все пути,
// которыми в цвет попадает число, идут через clamp: 300 становится 255,
// а -20 — нулём. Ближе всего к самому большому, а не случайное значение.
//
// Практика 9: цвет — класс с тремя каналами.
//
// В версии на C цвет был упакованным числом Pixel, а операции над ним —
// свободными функциями: pixel_adjust_brightness(pixel, delta). Здесь данные
// и операции вместе: color.brighten(40). Поля закрыты, и поменять канал
// в обход методов нельзя.
class Color {
public:
    static constexpr int CHANNEL_MAX = 255;

    // Цвет по трём каналам с насыщением: значение вне 0..255 приводится
    // к ближайшей границе. Статический метод принадлежит классу, а не
    // объекту, и вызывается через имя класса — Color::from_rgb(...).
    //
    // На практиках 9–10 параметры были std::uint8_t, и from_rgb(300, 0, 0)
    // молча давал канал 44: int превращался в uint8_t ещё при вызове,
    // до того как метод мог что-нибудь проверить. Теперь параметры — int,
    // и проверка внутри.
    static Color from_rgb(int red, int green, int blue);

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
