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

std::vector<std::pair<std::string, std::string>> MemoryEngine::pairs() const {
    return std::vector<std::pair<std::string, std::string>>(data_.begin(), data_.end());
}

std::string MemoryEngine::describe() const {
    return "Engine: in memory only, nothing is written to disk\n";
}
