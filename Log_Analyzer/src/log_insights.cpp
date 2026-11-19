#include "log_insights.hpp"

#include <algorithm>
#include <array>
#include <iterator>
#include <numeric>
#include <unordered_map>
#include <unordered_set>

#include "record_filter.hpp"

namespace {

// Предикат «ошибка», общий для двух запросов. Только отвечает.
bool is_error(const LogRecord& record) {
    return record.level() == Level::error || record.level() == Level::fatal;
}

} // namespace

double error_share(const LogBook& book) {
    if (book.count() == 0) {
        return 0.0;
    }
    // filter + reduce одним алгоритмом: count_if отбирает и считает,
    // не строя списка.
    long errors = count_matching(book, is_error);
    return static_cast<double>(errors) / static_cast<double>(book.count());
}

std::vector<std::pair<std::string, long>> noisiest_modules(const LogBook& book,
                                                           std::size_t limit) {
    // Группировка: модуль — сколько записей. Один проход по журналу.
    // Вложенный цикл «для каждого модуля пройти весь журнал» стоил бы
    // записей × модулей шагов; словарь — записей шагов.
    std::unordered_map<std::string, long> per_module;
    std::for_each(book.begin(), book.end(),
                  [&per_module](const LogRecord& record) { ++per_module[record.module()]; });

    std::vector<std::pair<std::string, long>> ranked(per_module.begin(), per_module.end());
    std::size_t shown = std::min(limit, ranked.size());
    // Частичная сортировка: упорядочить нужно только первые shown, а не все.
    // Словарь порядка не хранит, поэтому сортировать приходится в любом случае.
    std::partial_sort(ranked.begin(), ranked.begin() + static_cast<std::ptrdiff_t>(shown),
                      ranked.end(),
                      [](const std::pair<std::string, long>& a,
                         const std::pair<std::string, long>& b) {
                          return a.second > b.second || (a.second == b.second && a.first < b.first);
                      });
    ranked.resize(shown);
    return ranked;
}

bool mean_error_interval(const LogBook& book, double& seconds) {
    // filter: только ошибки. map: каждую — в секунды. Журнал может быть
    // не отсортирован, поэтому секунды сортируются.
    std::vector<const LogRecord*> errors = select(book, is_error);
    if (errors.size() < 2) {
        return false;   // интервалов нет: делить не на что
    }
    std::vector<long long> moments(errors.size());
    std::transform(errors.begin(), errors.end(), moments.begin(),
                   [](const LogRecord* record) { return record->time().seconds(); });
    std::sort(moments.begin(), moments.end());

    // Пары соседей без цикла со счётчиком: std::adjacent_difference пишет
    // на место каждого элемента его разность с предыдущим. Первый элемент
    // остаётся как был — он не интервал, и свёртка начинается со второго.
    std::vector<long long> gaps(moments.size());
    std::adjacent_difference(moments.begin(), moments.end(), gaps.begin());
    long long total = std::accumulate(gaps.begin() + 1, gaps.end(), 0LL);
    seconds = static_cast<double>(total) / static_cast<double>(gaps.size() - 1);
    return true;
}

bool busiest_hour(const LogBook& book, int& hour, long& records) {
    if (book.count() == 0) {
        return false;
    }
    // Группировка по часу — массив из 24 счётчиков: ключи — числа 0..23,
    // и словарь здесь не нужен. Час всегда в пределах: время записи
    // действительно по её инварианту (практика 11).
    std::array<long, 24> per_hour = {};
    std::for_each(book.begin(), book.end(),
                  [&per_hour](const LogRecord& record) { ++per_hour[record.time().hour()]; });
    // max_element при равенстве возвращает первый — то есть более ранний час.
    auto top = std::max_element(per_hour.begin(), per_hour.end());
    hour = static_cast<int>(top - per_hour.begin());
    records = *top;
    return true;
}

std::size_t distinct_modules(const LogBook& book) {
    // map в имена модулей прямо во множество: одинаковые имена схлопнутся.
    // Множество не помнит порядка, поэтому ответ от порядка записей
    // не зависит.
    std::unordered_set<std::string> names;
    std::transform(book.begin(), book.end(), std::inserter(names, names.end()),
                   [](const LogRecord& record) { return record.module(); });
    return names.size();
}
