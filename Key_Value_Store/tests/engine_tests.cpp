// Практика 12: проверки движков отдельной программой, для ctest.
//
// Один и тот же набор run_conformance прогоняется на обоих движках.
// Потом — проверка, которой в общем наборе нет: сохранность данных после
// закрытия и повторного открытия. Её обещает только журнальный движок,
// поэтому она и проверяется только у него.
//
//   cmake --build build && ctest --test-dir build

#include <cstdio>
#include <iostream>
#include <memory>
#include <string>

#include "conformance.hpp"
#include "engine.hpp"

int main() {
    const std::string path = "engine_tests.tmp.db";
    const std::string locations[] = { KvStore::IN_MEMORY, path };
    int failures = 0;
    for (const std::string& location : locations) {
        std::remove(path.c_str());
        std::unique_ptr<Engine> engine = make_engine(location);
        std::cout << "Engine for " << location << ":\n";
        failures += run_conformance(*engine);
    }

    std::cout << "Journal engine only: data survive reopening:\n";
    {
        std::unique_ptr<Engine> engine = make_engine(path);   // база после набора проверок
        std::string value;
        bool kept = engine->size() == 503 && engine->get("key999", value) == KvStatus::ok
                    && value == "998001";
        std::cout << (kept ? "  ok    " : "  FAIL  ") << "503 pairs after reopening\n";
        failures += kept ? 0 : 1;
    }
    std::remove(path.c_str());

    std::cout << (failures == 0 ? "All checks passed.\n" : "Some checks failed.\n");
    return failures == 0 ? 0 : 1;
}
