#include "filter.hpp"

#include <cctype>
#include <iostream>
#include <sstream>
#include <stdexcept>

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

void PixelFilter::apply(Image& image) const {
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            // transform — виртуальный вызов: что делать с цветом, решает
            // наследник, а обход один на всех.
            image.set_pixel(x, y, transform(image.pixel(x, y)));
        }
    }
}

std::string BrightnessFilter::name() const {
    return std::string("brightness ") + (delta_ >= 0 ? "+" : "") + std::to_string(delta_);
}

Color BrightnessFilter::transform(Color color) const {
    return color.brighten(delta_);   // color — копия, менять её можно
}

ContrastFilter::ContrastFilter(int percent)
    : percent_(percent) {
    if (percent < 0) {
        throw std::invalid_argument("contrast percent cannot be negative");
    }
}

std::string ContrastFilter::name() const {
    return "contrast " + std::to_string(percent_) + "%";
}

Color ContrastFilter::transform(Color color) const {
    // Умножение до деления, как везде. from_rgb насыщает: 128 + 200 = 255.
    constexpr int MIDDLE = 128;
    return Color::from_rgb(MIDDLE + (color.red() - MIDDLE) * percent_ / 100,
                           MIDDLE + (color.green() - MIDDLE) * percent_ / 100,
                           MIDDLE + (color.blue() - MIDDLE) * percent_ / 100);
}

void BlurFilter::apply(Image& image) const {
    // Копия исходного изображения: соседи читаются только из неё.
    // Копирующий конструктор Image (практика 10) делает свой буфер.
    const Image source = image;
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            int sum[3] = { 0, 0, 0 };
            int count = 0;
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
            image.set_pixel(x, y, Color::from_rgb(sum[0] / count, sum[1] / count,
                                                  sum[2] / count));
        }
    }
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
    if (name == "invert") {
        return std::make_unique<InvertFilter>();
    }
    if (name == "gray") {
        return std::make_unique<GrayFilter>();
    }
    if (name == "blur") {
        return std::make_unique<BlurFilter>();
    }
    int parameter = 0;
    if (name == "bright") {
        if (!read_parameter(words, position, "bright", "bright 40", parameter)) {
            return nullptr;
        }
        return std::make_unique<BrightnessFilter>(parameter);
    }
    if (name == "contrast") {
        if (!read_parameter(words, position, "contrast", "contrast 150", parameter)) {
            return nullptr;
        }
        if (parameter < 0) {
            std::cout << "The contrast percent cannot be negative\n";
            return nullptr;
        }
        return std::make_unique<ContrastFilter>(parameter);
    }
    std::cout << "Unknown filter: " << name << ". Known: " << filter_names() << '\n';
    return nullptr;
}

const char* filter_names() {
    return "invert, gray, bright <n>, contrast <percent>, blur";
}
