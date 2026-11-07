#pragma once

#include <string>
#include <utility>
#include <vector>

#include "store.hpp"

// Пары хранилища, упорядоченные по ключу (практика 4): сортировка слиянием
// и двоичный поиск границ диапазона. Хеш-таблица порядка не знает, поэтому
// для sorted и range пары копируются сюда и сортируются.
class SortedPairs {
public:
    // Копирует пары из хранилища и сортирует их. Возвращает наибольшую
    // глубину рекурсии сортировки.
    int build(const Store& store);

    int count() const { return static_cast<int>(pairs_.size()); }

    // Печатает все пары или пары с ключами от from до to включительно.
    // Возвращает, сколько напечатано.
    int print_all() const;
    int print_range(const std::string& from, const std::string& to) const;

private:
    void merge_sort(std::size_t left, std::size_t right, int depth, int& max_depth);
    void merge(std::size_t left, std::size_t middle, std::size_t right);
    // Первый номер в [low, high), ключ которого не меньше key (lower_bound)
    // или строго больше key (upper_bound).
    std::size_t lower_bound(std::size_t low, std::size_t high, const std::string& key) const;
    std::size_t upper_bound(std::size_t low, std::size_t high, const std::string& key) const;

    std::vector<std::pair<std::string, std::string>> pairs_;
    std::vector<std::pair<std::string, std::string>> buffer_;   // для слияния
};
