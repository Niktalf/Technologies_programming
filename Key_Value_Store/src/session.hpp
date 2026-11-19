#pragma once

#include <memory>
#include <string>

#include "engine.hpp"

// Два независимых хранилища в одной программе, каждое со своим движком.
// Команды работают с текущим.
//
// Практика 12: сессия хранит не хранилища, а указатели на абстрактный
// движок — std::unique_ptr<Engine>. Какой движок под каждым, решает
// make_engine по имени базы: «:memory:» — в памяти, иначе — журнал.
// Сессия и консоль этого не знают и не проверяют. Закрытое хранилище —
// пустой указатель.
//
// Практика 10: хранилища открываются конструктором сессии и закрываются
// её деструктором.
//
// Инвариант: 0 <= current_ < STORES; paths_[i] — имя, с которым хранилище
// номер i было открыто последний раз удачно. Неудачный open_current не
// меняет ни хранилище, ни имя.
class Session {
public:
    static constexpr int STORES = 2;

    // Открывает оба хранилища. Если второе не открылось, исключение уходит
    // из конструктора, а уже открытое первое закроет деструктор поля.
    Session(const std::string& first, const std::string& second);

    // Открыто ли хранилище: текущее, другое, номер index от 0.
    bool current_open() const { return stores_[current_] != nullptr; }
    bool other_open() const { return stores_[1 - current_] != nullptr; }
    bool is_open(int index) const { return stores_[index] != nullptr; }

    // Текущее, другое и хранилище номер index. Только для открытых:
    // проверять это — дело вызывающего.
    Engine& current() { return *stores_[current_]; }
    Engine& other() { return *stores_[1 - current_]; }
    const Engine& store(int index) const { return *stores_[index]; }
    int number() const { return current_ + 1; }

    // Закрывает текущее хранилище: указатель обнуляется, и unique_ptr
    // удаляет движок через виртуальный деструктор.
    void close_current();

    // Открывает на месте текущего базу location. Если открыть не удалось —
    // KvError, и текущее хранилище остаётся каким было.
    void open_current(const std::string& location);

    // Переключает текущее хранилище. false, если номер не от 1 до STORES.
    bool select(int number);

    const std::string& path(int index) const { return paths_[index]; }

private:
    std::unique_ptr<Engine> stores_[STORES];   // пустой указатель — хранилище закрыто
    std::string paths_[STORES];                // имена помнятся и после закрытия
    int current_ = 0;
};
