#pragma once

#include <string>

#include "level.hpp"
#include "status.hpp"

// Генератор тестовых журналов (практика 3). Тот же rand в том же порядке,
// что в версии на C: файл с тем же зерном совпадает байт в байт.
class LogGenerator {
public:
    LogStatus write(const std::string& path, long lines, unsigned int seed) const;

private:
    // Уровень с весами: отладочных и информационных сообщений больше всего,
    // критических — единицы.
    static Level random_level();
};
