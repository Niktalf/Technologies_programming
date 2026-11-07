#include "color.hpp"

namespace {

// Где лежит каждый канал в числе 0xRRGGBB.
constexpr int RED_SHIFT = 16;
constexpr int GREEN_SHIFT = 8;
constexpr std::uint32_t CHANNEL_MASK = 0xFFu;   // восемь младших битов: ровно один канал

} // namespace

Color Color::from_rgb(std::uint8_t red, std::uint8_t green, std::uint8_t blue) {
    Color color;
    color.red_ = red;   // закрытые поля доступны: метод принадлежит тому же классу
    color.green_ = green;
    color.blue_ = blue;
    return color;
}

Color Color::from_packed(std::uint32_t packed) {
    return from_rgb(static_cast<std::uint8_t>((packed >> RED_SHIFT) & CHANNEL_MASK),
                    static_cast<std::uint8_t>((packed >> GREEN_SHIFT) & CHANNEL_MASK),
                    static_cast<std::uint8_t>(packed & CHANNEL_MASK));
}

std::uint32_t Color::packed() const {
    // Канал приводится к uint32_t до сдвига: иначе после целочисленного
    // повышения он стал бы знаковым int.
    return (static_cast<std::uint32_t>(red_) << RED_SHIFT)
           | (static_cast<std::uint32_t>(green_) << GREEN_SHIFT) | blue_;
}

std::uint8_t Color::clamp(int value) {
    if (value < 0) {
        return 0;
    }
    if (value > CHANNEL_MAX) {
        return CHANNEL_MAX;
    }
    return static_cast<std::uint8_t>(value);
}

bool Color::same_as(Color other) const {
    return red_ == other.red_ && green_ == other.green_ && blue_ == other.blue_;
}

// Явный this. Вызов color.brighten(40) компилятор превращает в вызов
// функции, которой скрытым первым параметром передан адрес color —
// это и есть this. this->red_ — канал того самого цвета; обычно this->
// не пишут, здесь он оставлен для наглядности. *this — сам объект:
// возвращая ссылку на него, метод позволяет продолжить цепочку на том же
// объекте, а не на копии.
Color& Color::brighten(int amount) {
    // Вычисления идут в int: в uint8_t проверять насыщение уже поздно.
    this->red_ = clamp(this->red_ + amount);
    this->green_ = clamp(this->green_ + amount);
    this->blue_ = clamp(this->blue_ + amount);
    return *this;
}

Color& Color::darken(int amount) {
    return brighten(-amount);   // то же, что this->brighten(-amount)
}

std::uint8_t Color::luminance() const {
    // Веса 0,299, 0,587 и 0,114 взяты целыми числами с общим делителем 1000.
    // Умножение до деления: иначе целочисленное деление обнулило бы вклад
    // каждого канала. Формула та же, что pixel_luminance в версии на C.
    return clamp((299 * red_ + 587 * green_ + 114 * blue_) / 1000);
}

Color Color::to_gray() const {
    std::uint8_t level = luminance();
    return from_rgb(level, level, level);
}

Color Color::inverted() const {
    return from_rgb(static_cast<std::uint8_t>(CHANNEL_MAX - red_),
                    static_cast<std::uint8_t>(CHANNEL_MAX - green_),
                    static_cast<std::uint8_t>(CHANNEL_MAX - blue_));
}
