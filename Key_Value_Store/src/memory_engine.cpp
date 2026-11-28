#include "memory_engine.hpp"

KvStatus MemoryEngine::put(const std::string& key, const std::string& value, bool& existed) {
    existed = data_.count(key) > 0;
    if (!valid_pair(key, value)) {
        return KvStatus::bad_argument;
    }
    data_.insert_or_assign(key, value);
    return KvStatus::ok;
}

KvStatus MemoryEngine::get(const std::string& key, std::string& value) const {
    auto found = data_.find(key);
    if (found == data_.end()) {
        return KvStatus::not_found;
    }
    value = found->second;
    return KvStatus::ok;
}

KvStatus MemoryEngine::remove(const std::string& key) {
    if (!valid_pair(key, "")) {
        return KvStatus::bad_argument;
    }
    return data_.erase(key) > 0 ? KvStatus::ok : KvStatus::not_found;
}

long MemoryEngine::for_each(const Visitor& visit) const {
    // Цикл идёт, пока visit отвечает true. Ответ false прекращает обход:
    // остальные пары не перебираются вовсе.
    long visited = 0;
    bool going = true;
    auto pair = data_.begin();
    while (going && pair != data_.end()) {
        ++visited;
        going = visit(pair->first, pair->second);
        ++pair;
    }
    return visited;
}

std::string MemoryEngine::describe() const {
    return "Engine: in memory only, nothing is written to disk\n";
}
