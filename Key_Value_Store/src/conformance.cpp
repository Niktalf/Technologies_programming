#include "conformance.hpp"

#include <algorithm>
#include <iterator>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace {

// Печатает результат одной проверки и считает провалы.
void expect(bool passed, const char* what, int& failures) {
    std::cout << (passed ? "  ok    " : "  FAIL  ") << what << '\n';
    if (!passed) {
        ++failures;
    }
}

// Значение по ключу или пометка, что его нет.
std::string value_of(const Engine& engine, const std::string& key) {
    std::string value;
    return engine.get(key, value) == KvStatus::ok ? value : "<none>";
}

// Все пары движка, собранные обходом, — только для сравнения в проверках.
std::vector<std::pair<std::string, std::string>> collect(const Engine& engine) {
    std::vector<std::pair<std::string, std::string>> pairs;
    engine.for_each([&pairs](const std::string& key, const std::string& value) {
        pairs.emplace_back(key, value);
        return true;
    });
    return pairs;
}

} // namespace

int run_conformance(Engine& engine) {
    int failures = 0;
    bool existed = true;

    expect(engine.size() == 0 && collect(engine).empty(), "a new engine is empty", failures);

    KvStatus status = engine.put("alpha", "1", existed);
    expect(status == KvStatus::ok && !existed, "put of a new key: ok, did not exist", failures);
    expect(value_of(engine, "alpha") == "1", "get returns the stored value", failures);

    status = engine.put("alpha", "2", existed);
    expect(status == KvStatus::ok && existed && value_of(engine, "alpha") == "2"
               && engine.size() == 1,
           "put of an existing key replaces the value, size stays 1", failures);

    std::string missing;
    expect(engine.get("beta", missing) == KvStatus::not_found, "get of a missing key: not found",
           failures);
    expect(engine.remove("beta") == KvStatus::not_found, "remove of a missing key: not found",
           failures);

    expect(engine.remove("alpha") == KvStatus::ok && engine.size() == 0
               && value_of(engine, "alpha") == "<none>",
           "remove of an existing key: ok, the key is gone", failures);

    expect(engine.put("", "x", existed) == KvStatus::bad_argument, "empty key: bad argument",
           failures);
    std::string longest_key(KvStore::KEY_MAX, 'k');
    std::string longest_value(KvStore::VALUE_MAX, 'v');
    expect(engine.put(longest_key + "k", "x", existed) == KvStatus::bad_argument,
           "key one byte too long: bad argument", failures);
    expect(engine.put("k", longest_value + "v", existed) == KvStatus::bad_argument,
           "value one byte too long: bad argument", failures);
    expect(engine.size() == 0, "rejected pairs are not stored", failures);
    expect(engine.put(longest_key, longest_value, existed) == KvStatus::ok
               && value_of(engine, longest_key) == longest_value,
           "the longest key and value are accepted", failures);
    expect(engine.put("empty", "", existed) == KvStatus::ok && value_of(engine, "empty").empty(),
           "an empty value is a value", failures);

    // Любые байты, в том числе нулевой, кавычка и перевод строки.
    std::string odd("a\0b\"c\nd", 7);
    expect(engine.put("odd", odd, existed) == KvStatus::ok && value_of(engine, "odd") == odd,
           "any bytes in a value come back unchanged", failures);

    // Обход отдаёт ровно то, что лежит. Порядок не обещан, поэтому обе
    // стороны сортируются перед сравнением.
    std::vector<std::pair<std::string, std::string>> expected = {
        { longest_key, longest_value }, { "empty", "" }, { "odd", odd }
    };
    std::vector<std::pair<std::string, std::string>> actual = collect(engine);
    std::sort(expected.begin(), expected.end());
    std::sort(actual.begin(), actual.end());
    expect(actual == expected && engine.size() == 3, "for_each visits exactly the stored pairs",
           failures);

    // Досрочная остановка: функция обхода отвечает false на первой паре.
    // Обход должен на ней и закончиться, а не дозвать остальные.
    int calls = 0;
    long visited = engine.for_each([&calls](const std::string&, const std::string&) {
        ++calls;
        return false;
    });
    expect(calls == 1 && visited == 1, "for_each stops when the visitor returns false", failures);

    // Диапазон согласован с полным обходом: те же пары, отобранные по ключу.
    std::vector<std::pair<std::string, std::string>> in_range;
    engine.for_each_in_range("e", "p", [&in_range](const std::string& key,
                                                    const std::string& value) {
        in_range.emplace_back(key, value);
        return true;
    });
    std::sort(in_range.begin(), in_range.end());
    // Чего ждём: пары полного обхода (actual уже отсортирован) с ключами
    // в границах — "empty", "odd" и длинный ключ из букв k.
    std::vector<std::pair<std::string, std::string>> expected_range;
    std::copy_if(actual.begin(), actual.end(), std::back_inserter(expected_range),
                 [](const std::pair<std::string, std::string>& pair) {
                     return !(pair.first < "e") && !(std::string("p") < pair.first);
                 });
    expect(in_range == expected_range && in_range.size() == 3,
           "for_each_in_range gives the pairs of for_each within the bounds", failures);

    for (int i = 0; i < 1000; ++i) {
        engine.put("key" + std::to_string(i), std::to_string(i * i), existed);
    }
    for (int i = 0; i < 1000; i += 2) {
        engine.remove("key" + std::to_string(i));
    }
    expect(engine.size() == 503 && value_of(engine, "key999") == "998001"
               && value_of(engine, "key998") == "<none>",
           "1000 puts and 500 removes: 503 pairs, values in place", failures);

    expect(!engine.repaired(), "a new engine reports no damage", failures);
    return failures;
}
