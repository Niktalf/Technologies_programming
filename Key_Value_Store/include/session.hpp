#pragma once

#include <string>

#include "store.hpp"

// Два независимых хранилища в одной программе (практика 5), каждое
// со своим журналом (практика 8). Команды работают с текущим.
class Session {
public:
    static constexpr int STORES = 2;

    // Открывает оба хранилища. При ошибке ничего не остаётся открытым.
    KvStatus open(const std::string& first, const std::string& second);

    // Закрывает оба. Вызывается перед выходом: на этой практике объект
    // сам за собой не убирает.
    void close();

    // Текущее и другое хранилище, номер текущего от 1.
    Store& current() { return stores_[current_]; }
    Store& other() { return stores_[1 - current_]; }
    int number() const { return current_ + 1; }

    // Закрывает текущее хранилище и открывает на его месте журнал path.
    // Если открыть не удалось, текущее остаётся закрытым.
    KvStatus open_current(const std::string& path);

    // Переключает текущее хранилище. false, если номер не от 1 до STORES.
    bool select(int number);

    const std::string& path(int index) const { return paths_[index]; }

private:
    Store stores_[STORES];
    std::string paths_[STORES];   // пути помнятся и после close хранилища
    int current_ = 0;
};
