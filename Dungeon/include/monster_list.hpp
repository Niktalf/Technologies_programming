#pragma once

#include <vector>

#include "monster.hpp"

// Список монстров уровня.
//
// На практиках 7–8 растущий массив писался руками: удвоение ёмкости,
// временный указатель для realloc, освобождение. Здесь массив — std::vector:
// он растёт и освобождает память сам. Как это устроено внутри, разберём
// на практике 10, когда карта начнёт выделять память под клетки сама.
class MonsterList {
public:
    void clear() { monsters_.clear(); }

    // Добавляет монстра с полным здоровьем в клетку (x, y).
    void add(int x, int y);

    // Удаляет монстра номер index. На его место встаёт последний, поэтому
    // порядок остальных меняется, зато сдвигать хвост не нужно — как в C.
    void remove(int index);

    // Номер монстра в клетке (x, y) или -1, если там никого нет.
    int find(int x, int y) const;

    int count() const { return static_cast<int>(monsters_.size()); }

    // Монстр по номеру от 0 до count() - 1; проверять номер — дело
    // вызывающего, как и при обращении к массиву в C. Ссылка годится только
    // до следующего изменения списка: добавление может переложить вектор
    // в другое место памяти.
    Monster& at(int index) { return monsters_[static_cast<std::size_t>(index)]; }
    const Monster& at(int index) const { return monsters_[static_cast<std::size_t>(index)]; }

    // Число монстров и первые двадцать из них.
    void print() const;

private:
    std::vector<Monster> monsters_;
};
