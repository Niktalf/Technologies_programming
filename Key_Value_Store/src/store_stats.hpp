#pragma once

#include <functional>
#include <map>
#include <string>
#include <vector>

#include "engine.hpp"

// Практика 14: показатели базы — filter, map, reduce над парами движка.
//
// У движка одна операция обхода — for_each (практика 13). Поверх неё здесь
// три операции, из которых собираются все показатели:
//   filter — select_keys: ключи пар, для которых предикат отвечает «да»;
//   map    — map_pairs: каждую пару превратить в число;
//   reduce — reduce_pairs: свернуть пары в одно число с начальным значением.
// Работают с любым движком: им нужен только интерфейс Engine.

using PairPredicate = std::function<bool(const std::string& key, const std::string& value)>;
using PairValue = std::function<long long(const std::string& key, const std::string& value)>;
using PairStep = std::function<long long(long long so_far, const std::string& key,
                                         const std::string& value)>;

std::vector<std::string> select_keys(const Engine& engine, const PairPredicate& keep);
std::vector<long long> map_pairs(const Engine& engine, const PairValue& value);
long long reduce_pairs(const Engine& engine, long long initial, const PairStep& step);

// Показатели.
long long live_keys(const Engine& engine);
long long total_bytes(const Engine& engine);             // ключи и значения вместе
double mean_value_size(const Engine& engine);            // пустая база — 0
long long max_value_size(const Engine& engine);          // пустая база — 0
std::map<char, long> keys_by_first_char(const Engine& engine);
