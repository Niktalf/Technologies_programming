#include "image_stats.hpp"

#include <algorithm>
#include <iterator>
#include <numeric>
#include <unordered_map>
#include <utility>

#include "pixel_ops.hpp"

std::vector<Color> pixels(const Image& image) {
    std::vector<Color> colors;
    colors.reserve(static_cast<std::size_t>(image.width()) * image.height());
    visit_pixels(image, [&colors](Color color, int, int) { colors.push_back(color); });
    return colors;
}

std::vector<Color> select_pixels(const std::vector<Color>& colors, const ColorPredicate& keep) {
    std::vector<Color> selected;
    std::copy_if(colors.begin(), colors.end(), std::back_inserter(selected), keep);
    return selected;
}

std::vector<int> values(const std::vector<Color>& colors, const ColorValue& value) {
    std::vector<int> result(colors.size());
    std::transform(colors.begin(), colors.end(), result.begin(), value);
    return result;
}

Histogram::Histogram(const std::vector<Color>& colors) {
    // Один проход, три счётчика за шаг. Свёртка std::accumulate здесь
    // не годится: она копировала бы три массива по 256 чисел на каждую
    // точку. std::for_each с захватом гистограммы по ссылке — тот же
    // проход без копий: лямбда живёт только внутри вызова.
    std::for_each(colors.begin(), colors.end(), [this](Color c) {
        ++counts_[0][c.red()];
        ++counts_[1][c.green()];
        ++counts_[2][c.blue()];
    });
}

long Histogram::count(int channel, int level) const {
    return counts_[static_cast<std::size_t>(channel)][static_cast<std::size_t>(level)];
}

long Histogram::range(int channel, int first, int last) const {
    const std::array<long, LEVELS>& row = counts_[static_cast<std::size_t>(channel)];
    return std::accumulate(row.begin() + first, row.begin() + last + 1, 0L);
}

int Histogram::lowest(int channel) const {
    const std::array<long, LEVELS>& row = counts_[static_cast<std::size_t>(channel)];
    auto found = std::find_if(row.begin(), row.end(), [](long n) { return n > 0; });
    return found == row.end() ? 0 : static_cast<int>(found - row.begin());
}

int Histogram::highest(int channel) const {
    // Тот же поиск, но с конца: обратные итераторы идут от 255 к 0.
    const std::array<long, LEVELS>& row = counts_[static_cast<std::size_t>(channel)];
    auto found = std::find_if(row.rbegin(), row.rend(), [](long n) { return n > 0; });
    return found == row.rend() ? 0 : static_cast<int>(row.rend() - found) - 1;
}

bool luminance_range(const std::vector<int>& luminances, int& darkest, int& brightest) {
    if (luminances.empty()) {
        return false;
    }
    // Начальное значение — пара (наибольшая яркость, наименьшая яркость).
    // Минимум начинается с 255, а не с 0: иначе на любом изображении он
    // остался бы нулём. Максимум — наоборот, с 0.
    std::pair<int, int> found = std::accumulate(
        luminances.begin(), luminances.end(), std::make_pair(255, 0),
        [](std::pair<int, int> so_far, int value) {
            return std::make_pair(std::min(so_far.first, value), std::max(so_far.second, value));
        });
    darkest = found.first;
    brightest = found.second;
    return true;
}

double mean(const std::vector<int>& numbers) {
    if (numbers.empty()) {
        return 0.0;
    }
    // 0LL: сумма копится в long long. Яркость 255 на 8192 * 8192 точках —
    // 17 миллиардов, в int это не помещается. Деление — в double: в целых
    // 127,6 стало бы 127.
    long long sum = std::accumulate(numbers.begin(), numbers.end(), 0LL);
    return static_cast<double>(sum) / static_cast<double>(numbers.size());
}

bool most_frequent(const std::vector<Color>& colors, Color& color, long& times) {
    if (colors.empty()) {
        return false;
    }
    // Группировка через словарь: упакованный цвет — сколько раз встретился.
    // Один проход, а не вложенный цикл «для каждого цвета посчитать его
    // по всему изображению».
    std::unordered_map<std::uint32_t, long> counts;
    std::for_each(colors.begin(), colors.end(), [&counts](Color c) { ++counts[c.packed()]; });
    // Наибольший счётчик; при равенстве — меньший упакованный цвет, чтобы
    // ответ не зависел от порядка хеш-таблицы.
    auto best = std::max_element(counts.begin(), counts.end(),
                                 [](const std::pair<const std::uint32_t, long>& a,
                                    const std::pair<const std::uint32_t, long>& b) {
                                     return a.second < b.second
                                            || (a.second == b.second && a.first > b.first);
                                 });
    color = Color::from_packed(best->first);
    times = best->second;
    return true;
}
