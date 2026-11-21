#include "conformance.hpp"

#include <algorithm>
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

} // namespace

int run_conformance(Engine& engine) {
    int failures = 0;
    bool existed = true;

    expect(engine.size() == 0 && engine.pairs().empty(), "a new engine is empty", failures);

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
    std::vector<std::pair<std::string, std::string>> actual = engine.pairs();
    std::sort(expected.begin(), expected.end());
    std::sort(actual.begin(), actual.end());
    expect(actual == expected && engine.size() == 3, "pairs returns exactly the stored pairs",
           failures);

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
