#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <vector>

#include "color.hpp"
#include "image.hpp"

// Практика 14: анализ изображения — filter, map, reduce.
//
// Изображение сначала превращается в последовательность цветов (pixels),
// а дальше с ней работают три операции стандартной библиотеки:
//   filter — std::copy_if: оставить точки, для которых предикат отвечает «да»;
//   map    — std::transform: каждую точку превратить в число;
//   reduce — std::accumulate: свернуть числа в одно значение.
// Ни один запрос не написан циклом с переменной-накопителем.

// Предикат точки: только отвечает, ничего не меняет и не печатает.
using ColorPredicate = std::function<bool(Color color)>;

// Число для каждой точки: яркость, канал.
using ColorValue = std::function<int(Color color)>;

// Все точки изображения строками сверху вниз.
std::vector<Color> pixels(const Image& image);

// filter: точки, для которых keep отвечает «да».
std::vector<Color> select_pixels(const std::vector<Color>& colors, const ColorPredicate& keep);

// map: число value(точка) для каждой точки.
std::vector<int> values(const std::vector<Color>& colors, const ColorValue& value);

// Гистограмма трёх каналов: сколько точек с каждым значением канала.
// Считается за один проход по точкам — для всех трёх каналов сразу.
class Histogram {
public:
    static constexpr int LEVELS = 256;

    explicit Histogram(const std::vector<Color>& colors);

    // Сколько точек со значением level в канале: 0 — красный, 1 — зелёный,
    // 2 — синий.
    long count(int channel, int level) const;

    // Сумма счётчиков от first до last включительно — свёрткой.
    long range(int channel, int first, int last) const;

    // Наименьшее и наибольшее значение канала, которое встретилось хоть раз.
    // У пустой гистограммы — 0 и 0.
    int lowest(int channel) const;
    int highest(int channel) const;

private:
    std::array<std::array<long, LEVELS>, 3> counts_ = {};
};

// Наименьшая и наибольшая яркость за один проход — одна свёртка с парой
// в накопителе. false у пустого набора.
bool luminance_range(const std::vector<int>& luminances, int& darkest, int& brightest);

// Средняя яркость в double: сумма свёрткой в long long, деление — в double.
// У пустого набора — 0.
double mean(const std::vector<int>& numbers);

// Самый частый цвет и сколько раз он встретился. false у пустого набора.
bool most_frequent(const std::vector<Color>& colors, Color& color, long& times);
