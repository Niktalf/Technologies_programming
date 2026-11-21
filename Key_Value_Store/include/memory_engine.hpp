#pragma once

#include <string>
#include <unordered_map>

#include "engine.hpp"

// Движок в памяти: та же хеш-таблица, что у журнального, но без журнала.
// Быстрый, а после завершения программы данные пропадают. Подходит для
// временных данных и для проверок.
//
// Инвариант: в таблице только пары с допустимыми длинами ключа и значения.
class MemoryEngine : public Engine {
public:
    MemoryEngine() = default;

    KvStatus put(const std::string& key, const std::string& value, bool& existed) override;
    KvStatus get(const std::string& key, std::string& value) const override;
    KvStatus remove(const std::string& key) override;
    std::vector<std::pair<std::string, std::string>> pairs() const override;
    std::size_t size() const override { return data_.size(); }
    bool repaired() const override { return false; }
    std::string describe() const override;

private:
    std::unordered_map<std::string, std::string> data_;
};
