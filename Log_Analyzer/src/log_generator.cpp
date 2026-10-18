#include "log_generator.hpp"

#include <cstdlib>
#include <fstream>
#include <iomanip>

namespace {

const char* const MODULES[] = { "net", "db", "ui", "auth", "cache", "disk" };
const char* const MESSAGES[] = {
    "request processed",
    "connection established",
    "connection lost",
    "timeout exceeded",
    "user|admin logged in",
    "configuration file not found",
    "retrying in 5 seconds",
    "cache cleared"
};

} // namespace

Level LogGenerator::random_level() {
    int r = std::rand() % 100;
    if (r < 30) {
        return Level::debug;
    }
    if (r < 70) {
        return Level::info;
    }
    if (r < 85) {
        return Level::warn;
    }
    if (r < 97) {
        return Level::error;
    }
    return Level::fatal;
}

LogStatus LogGenerator::write(const std::string& path, long lines, unsigned int seed) const {
    if (lines < 0) {
        return LogStatus::bad_argument;
    }
    std::ofstream file(path);
    if (!file) {
        return LogStatus::cannot_open;
    }
    std::srand(seed);
    const int module_count = static_cast<int>(sizeof MODULES / sizeof MODULES[0]);
    const int message_count = static_cast<int>(sizeof MESSAGES / sizeof MESSAGES[0]);
    file << std::setfill('0');
    for (long i = 0; i < lines && file; ++i) {
        // Каждое случайное значение берётся отдельной инструкцией: порядок
        // вычисления частей одного выражения не определён, а так файл
        // с тем же зерном совпадает на любом компиляторе.
        int day = 1 + std::rand() % 28;
        int hour = std::rand() % 24;
        int minute = std::rand() % 60;
        int second = std::rand() % 60;
        Level level = random_level();
        const char* module = MODULES[std::rand() % module_count];
        const char* message = MESSAGES[std::rand() % message_count];
        file << "2026-03-" << std::setw(2) << day << ' ' << std::setw(2) << hour << ':'
             << std::setw(2) << minute << ':' << std::setw(2) << second << '|'
             << level_name(level) << '|' << module << '|' << message << " #" << i + 1 << '\n';
    }
    // Часть ошибок записи видна только при закрытии: данные из буфера
    // уходят на диск именно тогда.
    file.close();
    return file ? LogStatus::ok : LogStatus::write_failed;
}
