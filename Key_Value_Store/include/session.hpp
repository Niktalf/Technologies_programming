#pragma once

#include <string>

#include "store.hpp"

// Два независимых хранилища в одной программе, каждое со своим журналом.
// Команды работают с текущим.
//
// Практика 10: хранилища открываются конструктором сессии и закрываются
// её деструктором. Закрыть или переоткрыть одно хранилище — значит
// переместить на его место новый объект: пустой или открытый.
class Session {
public:
    static constexpr int STORES = 2;

    // Открывает оба хранилища. Если второе не открылось, исключение уходит
    // из конструктора, а уже открытое первое закроет деструктор поля.
    Session(const std::string& first, const std::string& second);

    // Текущее и другое хранилище, номер текущего от 1.
    Store& current() { return stores_[current_]; }
    Store& other() { return stores_[1 - current_]; }
    int number() const { return current_ + 1; }

    // Закрывает текущее хранилище: на его место перемещается пустое.
    void close_current();

    // Открывает на месте текущего журнал path. Если открыть не удалось —
    // KvError, и текущее хранилище остаётся каким было.
    void open_current(const std::string& path);

    // Переключает текущее хранилище. false, если номер не от 1 до STORES.
    bool select(int number);

    const std::string& path(int index) const { return paths_[index]; }

private:
    Store stores_[STORES];        // сначала пустые, конструктор открывает оба
    std::string paths_[STORES];   // пути помнятся и после закрытия хранилища
    int current_ = 0;
};
