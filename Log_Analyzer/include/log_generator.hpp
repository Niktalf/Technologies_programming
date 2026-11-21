#pragma once

#include <string>

#include "level.hpp"
#include "log_record.hpp"
#include "status.hpp"

// Генератор тестовых журналов (практика 3). Тот же rand в том же порядке,
// что в версии на C: файл с тем же зерном совпадает байт в байт.
class LogGenerator {
public:
    LogStatus write(const std::string& path, long lines, unsigned int seed) const;

    // Практика 12: одна случайная запись с номером number. Её берут и write,
    // и источник-генератор RandomSource. Случайные числа — из std::rand,
    // зерно задаёт вызывающий через std::srand.
    static LogRecord make_record(long number);

private:
    // Уровень с весами: отладочных и информационных сообщений больше всего,
    // критических — единицы.
    static Level random_level();
};
