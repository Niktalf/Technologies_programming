#include "log_stats.hpp"

#include <iomanip>
#include <iostream>

namespace {

constexpr long BAR_WIDTH = 50;   // длина самого длинного столбика в hours

} // namespace

std::size_t LogStats::module_index(const std::string& module) {
    for (std::size_t i = 0; i < modules_.size(); ++i) {
        if (modules_[i] == module) {
            return i;
        }
    }
    modules_.push_back(module);
    module_counts_.push_back(0);
    return modules_.size() - 1;
}

void LogStats::collect(const LogBook& book) {
    // Объект может собирать статистику повторно: сначала всё обнуляем.
    total_ = book.count();
    for (long& value : by_level_) {
        value = 0;
    }
    for (long& value : by_hour_) {
        value = 0;
    }
    modules_.clear();
    module_counts_.clear();

    // Один проход — вся статистика.
    for (long i = 0; i < book.count(); ++i) {
        const LogRecord& record = book.at(i);
        ++by_level_[static_cast<int>(record.level())];
        int hour = record.time().hour();
        if (hour >= 0 && hour < 24) {
            ++by_hour_[hour];
        }
        ++module_counts_[module_index(record.module())];
    }
}

void LogStats::print_levels() const {
    for (int i = 0; i < LEVEL_COUNT; ++i) {
        // Доля в процентах: умножаем до деления, в целых числах.
        long percent = total_ > 0 ? by_level_[i] * 100 / total_ : 0;
        std::cout << "  " << std::left << std::setw(5) << level_name(static_cast<Level>(i))
                  << std::right << ' ' << std::setw(7) << by_level_[i] << "  " << std::setw(3)
                  << percent << "%\n";
    }
}

void LogStats::print_hours() const {
    long peak = 0;
    for (long value : by_hour_) {
        if (value > peak) {
            peak = value;
        }
    }
    for (int h = 0; h < 24; ++h) {
        // Длина столбика пропорциональна числу записей. Если записей нет
        // вовсе, делить на ноль нельзя.
        long bar = peak > 0 ? by_hour_[h] * BAR_WIDTH / peak : 0;
        std::cout << "  " << std::setfill('0') << std::setw(2) << h << std::setfill(' ') << "  "
                  << std::setw(6) << by_hour_[h] << "  "
                  << std::string(static_cast<std::size_t>(bar), '#') << '\n';
    }
}

void LogStats::print_modules() const {
    for (std::size_t i = 0; i < modules_.size(); ++i) {
        std::cout << "  " << std::left << std::setw(12) << modules_[i] << std::right << ' '
                  << std::setw(7) << module_counts_[i] << '\n';
    }
}
