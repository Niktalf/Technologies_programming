#include "engine.hpp"

#include "journal_engine.hpp"
#include "memory_engine.hpp"

bool Engine::valid_pair(const std::string& key, const std::string& value) {
    return !key.empty() && key.size() <= KvStore::KEY_MAX && value.size() <= KvStore::VALUE_MAX;
}

std::unique_ptr<Engine> make_engine(const std::string& location) {
    if (location == KvStore::IN_MEMORY) {
        return std::make_unique<MemoryEngine>();
    }
    return std::make_unique<JournalEngine>(location);
}

long Engine::for_each_in_range(const std::string& from, const std::string& to,
                               const Visitor& visit) const {
    // Обход диапазона — обход всех пар с фильтром по ключу. Пары вне
    // диапазона visit не получает, а его ответ «стоп» передаётся наружу.
    // Захват по ссылке безопасен: лямбда живёт только внутри вызова
    // for_each, а from, to и visit — дольше.
    long visited = 0;
    for_each([&](const std::string& key, const std::string& value) {
        if (key < from || to < key) {
            return true;   // не наша пара — идём дальше
        }
        ++visited;
        return visit(key, value);
    });
    return visited;
}
