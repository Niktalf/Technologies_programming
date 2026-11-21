#include "log_report.hpp"

#include <iomanip>
#include <iostream>

namespace {

constexpr long BAR_WIDTH = 50;   // длина самого длинного столбика в часах

} // namespace

void LevelReport::accept(const LogRecord& record) {
    // Индекс без проверки: уровень записи всегда один из пяти —
    // это инвариант LogRecord (практика 11).
    ++by_level_[static_cast<int>(record.level())];
    ++total_;
}

void LevelReport::print() const {
    for (int i = 0; i < LEVEL_COUNT; ++i) {
        // Доля в процентах: умножаем до деления, в целых числах.
        long percent = total_ > 0 ? by_level_[i] * 100 / total_ : 0;
        std::cout << "  " << std::left << std::setw(5) << level_name(static_cast<Level>(i))
                  << std::right << ' ' << std::setw(7) << by_level_[i] << "  " << std::setw(3)
                  << percent << "%\n";
    }
}

void ModuleReport::accept(const LogRecord& record) {
    std::size_t i = 0;
    while (i < modules_.size() && modules_[i] != record.module()) {
        ++i;
    }
    if (i == modules_.size()) {
        modules_.push_back(record.module());
        counts_.push_back(0);
    }
    ++counts_[i];
}

void ModuleReport::print() const {
    // Выбор наибольшего: каждый раз ищем самый частый из ещё не показанных.
    // При равенстве первым идёт тот, кто раньше появился в журнале.
    // Модулей единицы, поэтому квадратичный перебор здесь не беда.
    std::vector<bool> shown(modules_.size(), false);
    int printed = 0;
    while (printed < TOP && printed < static_cast<int>(modules_.size())) {
        std::size_t best = modules_.size();
        for (std::size_t i = 0; i < modules_.size(); ++i) {
            if (!shown[i] && (best == modules_.size() || counts_[i] > counts_[best])) {
                best = i;
            }
        }
        shown[best] = true;
        std::cout << "  " << std::left << std::setw(12) << modules_[best] << std::right << ' '
                  << std::setw(7) << counts_[best] << '\n';
        ++printed;
    }
    if (modules_.size() > static_cast<std::size_t>(TOP)) {
        std::cout << "  ... and " << modules_.size() - static_cast<std::size_t>(TOP) << " more\n";
    }
}

void HourReport::accept(const LogRecord& record) {
    // Время записи действительно (инвариант LogRecord), значит час от 0 до 23.
    ++by_hour_[record.time().hour()];
}

void HourReport::print() const {
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

std::unique_ptr<LogReport> make_report(const std::string& name) {
    if (name == "levels") {
        return std::make_unique<LevelReport>();
    }
    if (name == "modules") {
        return std::make_unique<ModuleReport>();
    }
    if (name == "hours") {
        return std::make_unique<HourReport>();
    }
    return nullptr;
}

long run_reports(LogSource& source, const std::vector<std::unique_ptr<LogReport>>& reports) {
    long count = 0;
    std::unique_ptr<LogRecord> record = source.next();   // виртуальный вызов
    while (record) {
        for (const std::unique_ptr<LogReport>& report : reports) {
            report->accept(*record);                     // и здесь
        }
        ++count;
        record = source.next();
    }
    return count;
}
