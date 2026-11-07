#include "sorted_pairs.hpp"

#include <iostream>

int SortedPairs::build(const Store& store) {
    pairs_ = store.pairs();
    buffer_.resize(pairs_.size());
    int depth = 0;
    merge_sort(0, pairs_.size(), 1, depth);
    return depth;
}

void SortedPairs::merge(std::size_t left, std::size_t middle, std::size_t right) {
    std::size_t i = left;
    std::size_t j = middle;
    std::size_t k = left;
    while (i < middle && j < right) {
        // std::string сравнивается по содержимому: compare — то же, что strcmp.
        if (pairs_[i].first.compare(pairs_[j].first) <= 0) {
            buffer_[k] = pairs_[i];
            ++i;
        } else {
            buffer_[k] = pairs_[j];
            ++j;
        }
        ++k;
    }
    // Одна из половин кончилась; остаток другой уже упорядочен.
    while (i < middle) {
        buffer_[k] = pairs_[i];
        ++i;
        ++k;
    }
    while (j < right) {
        buffer_[k] = pairs_[j];
        ++j;
        ++k;
    }
    for (std::size_t n = left; n < right; ++n) {
        pairs_[n] = buffer_[n];
    }
}

void SortedPairs::merge_sort(std::size_t left, std::size_t right, int depth, int& max_depth) {
    if (depth > max_depth) {
        max_depth = depth;
    }
    if (right - left < 2) {
        return;   // ноль или один элемент уже упорядочены
    }
    std::size_t middle = left + (right - left) / 2;
    merge_sort(left, middle, depth + 1, max_depth);
    merge_sort(middle, right, depth + 1, max_depth);
    merge(left, middle, right);
}

std::size_t SortedPairs::lower_bound(std::size_t low, std::size_t high,
                                     const std::string& key) const {
    if (low >= high) {
        return low;
    }
    std::size_t middle = low + (high - low) / 2;
    if (pairs_[middle].first.compare(key) < 0) {
        return lower_bound(middle + 1, high, key);
    }
    return lower_bound(low, middle, key);
}

// Разница с lower_bound — одно сравнение, и она решает, войдёт ли правая
// граница в диапазон.
std::size_t SortedPairs::upper_bound(std::size_t low, std::size_t high,
                                     const std::string& key) const {
    if (low >= high) {
        return low;
    }
    std::size_t middle = low + (high - low) / 2;
    if (pairs_[middle].first.compare(key) <= 0) {
        return upper_bound(middle + 1, high, key);
    }
    return upper_bound(low, middle, key);
}

int SortedPairs::print_all() const {
    for (const auto& pair : pairs_) {
        std::cout << pair.first << " = " << pair.second << '\n';
    }
    return count();
}

int SortedPairs::print_range(const std::string& from, const std::string& to) const {
    std::size_t first = lower_bound(0, pairs_.size(), from);
    std::size_t last = upper_bound(0, pairs_.size(), to);
    for (std::size_t i = first; i < last; ++i) {
        std::cout << pairs_[i].first << " = " << pairs_[i].second << '\n';
    }
    return static_cast<int>(last - first);
}
