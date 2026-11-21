#include "log_generator.hpp"

#include <cstdlib>
#include <fstream>
#include <string>

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

LogRecord LogGenerator::make_record(long number) {
    const int module_count = static_cast<int>(sizeof MODULES / sizeof MODULES[0]);
    const int message_count = static_cast<int>(sizeof MESSAGES / sizeof MESSAGES[0]);
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
    return LogRecord(Timestamp::from_parts(2026, 3, day, hour, minute, second), level, module,
                     std::string(message) + " #" + std::to_string(number));
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
    for (long i = 0; i < lines && file; ++i) {
        LogRecord record = make_record(i + 1);
        file << record.time().text() << '|' << level_name(record.level()) << '|'
             << record.module() << '|' << record.text() << '\n';
    }
    // Часть ошибок записи видна только при закрытии: данные из буфера
    // уходят на диск именно тогда.
    file.close();
    return file ? LogStatus::ok : LogStatus::write_failed;
}
