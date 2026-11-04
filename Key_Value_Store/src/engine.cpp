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
