#include "filter.hpp"

#include <cctype>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace {

// Целое число из слова: всё слово — число.
bool parse_int(const std::string& word, int& out) {
    std::istringstream in(word);
    int value = 0;
    if (!(in >> value) || !in.eof()) {
        return false;
    }
    out = value;
    return true;
}

// Число-параметр фильтра name из words[position]. При ошибке печатает пример.
bool read_parameter(const std::vector<std::string>& words, std::size_t& position,
                    const char* name, const char* example, int& out) {
    if (position >= words.size() || !parse_int(words[position], out)) {
        std::cout << name << " needs a number, e.g. " << example << '\n';
        return false;
    }
    ++position;
    return true;
}

} // namespace

PixelFilter::PixelFilter(std::string name, PixelOp op)
    : name_(std::move(name)), op_(std::move(op)) {
    if (!op_) {
        throw std::invalid_argument("a pixel filter needs an operation");
    }
}

VignetteFilter::VignetteFilter(int strength)
    : strength_(strength) {
    if (strength < 0 || strength > 100) {
        throw std::invalid_argument("vignette strength must be from 0 to 100");
    }
}

std::string VignetteFilter::name() const {
    return "vignette " + std::to_string(strength_) + "%";
}

void VignetteFilter::apply(Image& image) const {
    // Центр и квадрат расстояния до угла — числа одного изображения: они
    // считаются один раз и захватываются в лямбду по значению.
    double cx = (image.width() - 1) / 2.0;
    double cy = (image.height() - 1) / 2.0;
    double corner = cx * cx + cy * cy;
    double strength = strength_ / 100.0;
    for_each_pixel_at(image, [cx, cy, corner, strength](Color c, int x, int y) {
        // Доля расстояния до угла, в квадрате: в центре 0, в углу 1.
        // Квадрат даёт мягкий переход: середина почти не темнеет.
        double share = corner > 0 ? ((x - cx) * (x - cx) + (y - cy) * (y - cy)) / corner : 0;
        double keep = 1.0 - strength * share;
        return Color::from_rgb(static_cast<int>(c.red() * keep),
                               static_cast<int>(c.green() * keep),
                               static_cast<int>(c.blue() * keep));
    });
}

void BlurFilter::apply(Image& image) const {
    // Копия исходного изображения: соседи читаются только из неё.
    // Копирующий конструктор Image (практика 10) делает свой буфер.
    const Image source = image;
    for_each_pixel_at(image, [&source](Color, int x, int y) {
        int sum[3] = { 0, 0, 0 };
        int count = 0;
        // Квадрат 3 на 3 — маленький обход соседей, не изображения.
        for (int dy = -1; dy <= 1; ++dy) {
            for (int dx = -1; dx <= 1; ++dx) {
                if (source.inside(x + dx, y + dy)) {
                    Color c = source.pixel(x + dx, y + dy);
                    sum[0] += c.red();
                    sum[1] += c.green();
                    sum[2] += c.blue();
                    ++count;
                }
            }
        }
        return Color::from_rgb(sum[0] / count, sum[1] / count, sum[2] / count);
    });
}

std::unique_ptr<Filter> make_filter(const std::vector<std::string>& words,
                                    std::size_t& position) {
    if (position >= words.size()) {
        return nullptr;
    }
    // Регистр не важен, как и у команд редактора: BLUR — то же, что blur.
    std::string name = words[position];
    for (char& c : name) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    ++position;
    // Четыре попиксельных фильтра — четыре лямбды. Без захвата — те, у кого
    // нет параметра; с захватом по значению — те, у кого есть: лямбда
    // хранит свою копию числа, и make_filter может спокойно закончиться.
    if (name == "invert") {
        return std::make_unique<PixelFilter>("invert", [](Color c) { return c.inverted(); });
    }
    if (name == "gray") {
        return std::make_unique<PixelFilter>("gray", [](Color c) { return c.to_gray(); });
    }
    if (name == "blur") {
        return std::make_unique<BlurFilter>();
    }
    int parameter = 0;
    if (name == "bright") {
        if (!read_parameter(words, position, "bright", "bright 40", parameter)) {
            return nullptr;
        }
        std::string title = std::string("brightness ") + (parameter >= 0 ? "+" : "")
                            + std::to_string(parameter);
        return std::make_unique<PixelFilter>(title, [parameter](Color c) {
            return c.brighten(parameter);   // c — копия, менять её можно
        });
    }
    if (name == "contrast") {
        if (!read_parameter(words, position, "contrast", "contrast 150", parameter)) {
            return nullptr;
        }
        if (parameter < 0) {
            std::cout << "The contrast percent cannot be negative\n";
            return nullptr;
        }
        // Каждый канал отдаляется от середины 128 или приближается к ней.
        // Умножение до деления; from_rgb насыщает: 128 + 200 = 255.
        return std::make_unique<PixelFilter>(
            "contrast " + std::to_string(parameter) + "%", [parameter](Color c) {
                constexpr int MIDDLE = 128;
                return Color::from_rgb(MIDDLE + (c.red() - MIDDLE) * parameter / 100,
                                       MIDDLE + (c.green() - MIDDLE) * parameter / 100,
                                       MIDDLE + (c.blue() - MIDDLE) * parameter / 100);
            });
    }
    if (name == "vignette") {
        if (!read_parameter(words, position, "vignette", "vignette 60", parameter)) {
            return nullptr;
        }
        if (parameter < 0 || parameter > 100) {
            std::cout << "The vignette strength is from 0 to 100\n";
            return nullptr;
        }
        return std::make_unique<VignetteFilter>(parameter);
    }
    std::cout << "Unknown filter: " << name << ". Known: " << filter_names() << '\n';
    return nullptr;
}

const char* filter_names() {
    return "invert, gray, bright <n>, contrast <percent>, vignette <percent>, blur";
}
