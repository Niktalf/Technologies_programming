#include "store_stats.hpp"

#include <algorithm>
#include <numeric>

std::vector<std::string> select_keys(const Engine& engine, const PairPredicate& keep) {
    std::vector<std::string> keys;
    engine.for_each([&](const std::string& key, const std::string& value) {
        if (keep(key, value)) {
            keys.push_back(key);
        }
        return true;
    });
    return keys;
}

std::vector<long long> map_pairs(const Engine& engine, const PairValue& value) {
    std::vector<long long> values;
    values.reserve(engine.size());
    engine.for_each([&](const std::string& key, const std::string& stored) {
        values.push_back(value(key, stored));
        return true;
    });
    return values;
}

// Свёртка по движку — то же, что std::accumulate, но источник — обход
// for_each, а не пара итераторов: итераторов движок не даёт (практика 13).
// Накопитель живёт здесь, в одном месте; показатели передают только шаг.
long long reduce_pairs(const Engine& engine, long long initial, const PairStep& step) {
    long long so_far = initial;
    engine.for_each([&](const std::string& key, const std::string& value) {
        so_far = step(so_far, key, value);
        return true;
    });
    return so_far;
}

long long live_keys(const Engine& engine) {
    // filter + reduce: каждая пара, прошедшая условие, добавляет единицу.
    // Условие здесь — «любая пара»: в таблице только живые ключи.
    return reduce_pairs(engine, 0, [](long long count, const std::string&, const std::string&) {
        return count + 1;
    });
}

long long total_bytes(const Engine& engine) {
    std::vector<long long> sizes = map_pairs(engine, [](const std::string& key,
                                                        const std::string& value) {
        return static_cast<long long>(key.size() + value.size());
    });
    return std::accumulate(sizes.begin(), sizes.end(), 0LL);
}

double mean_value_size(const Engine& engine) {
    std::vector<long long> sizes = map_pairs(engine, [](const std::string&,
                                                        const std::string& value) {
        return static_cast<long long>(value.size());
    });
    if (sizes.empty()) {
        return 0.0;   // пустая база: значений нет, делить не на что
    }
    return static_cast<double>(std::accumulate(sizes.begin(), sizes.end(), 0LL))
           / static_cast<double>(sizes.size());
}

long long max_value_size(const Engine& engine) {
    // Начальное значение 0 здесь верно: размер не бывает отрицательным,
    // и 0 — наименьший возможный. Для яркости или времени так было бы нельзя.
    return reduce_pairs(engine, 0, [](long long largest, const std::string&,
                                      const std::string& value) {
        return std::max(largest, static_cast<long long>(value.size()));
    });
}

std::map<char, long> keys_by_first_char(const Engine& engine) {
    // map: каждый ключ — в его первый символ. Потом группировка словарём.
    // std::map, а не unordered_map: распределение печатается по порядку
    // символов. Ключ не бывает пустым — это проверяет движок.
    std::vector<long long> firsts = map_pairs(engine, [](const std::string& key,
                                                         const std::string&) {
        return static_cast<long long>(static_cast<unsigned char>(key[0]));
    });
    std::map<char, long> groups;
    std::for_each(firsts.begin(), firsts.end(),
                  [&groups](long long c) { ++groups[static_cast<char>(c)]; });
    return groups;
}
