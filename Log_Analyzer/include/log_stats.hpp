#pragma once

#include <string>
#include <vector>

#include "log_book.hpp"

// Статистика журнала: записи по уровням, по часам и по модулям. Собирается
// за один проход. На практике 3 таблица модулей была массивом на 64 имени
// с проверкой переполнения; здесь это вектор, и места хватает всем.
class LogStats {
public:
    void collect(const LogBook& book);

    void print_levels() const;
    void print_hours() const;
    void print_modules() const;

private:
    // Номер модуля в таблице; если его нет — добавляет.
    std::size_t module_index(const std::string& module);

    long total_ = 0;
    long by_level_[LEVEL_COUNT] = {};
    long by_hour_[24] = {};
    std::vector<std::string> modules_;   // в порядке первого появления
    std::vector<long> module_counts_;    // module_counts_[i] — записей модуля modules_[i]
};
