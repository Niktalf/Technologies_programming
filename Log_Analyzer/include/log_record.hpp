#pragma once

#include <string>

#include "level.hpp"
#include "timestamp.hpp"

// Запись журнала.
//
// Инвариант:
//   * time() — действительный момент: год от Timestamp::MIN_YEAR
//     до Timestamp::MAX_YEAR, поэтому записи всегда можно упорядочить;
//   * level() — один из пяти известных уровней;
//   * module() не пуст.
// Конструктор проверяет всё это и при нарушении бросает
// std::invalid_argument — записи нет. Изменяющих методов у записи нет
// вовсе: после создания она только читается. Поэтому весь остальной код —
// отчёты, выборки, сортировка — не проверяет эти вещи заново.
//
// Практика 11: запись больше не разбирает текст. На практике 10 у неё были
// parse(line) и assign(...), и запись умела быть «пустой», ещё не
// заполненной. Разбор строки журнала — дело класса LogParser, двоичного
// файла — класса BinaryLog. Запись не знает, откуда её взяли.
class LogRecord {
public:
    LogRecord(Timestamp time, Level level, std::string module, std::string text);

    Timestamp time() const { return time_; }
    Level level() const { return level_; }
    const std::string& module() const { return module_; }
    const std::string& text() const { return text_; }

    // Печатает запись одной строкой.
    void print() const;

    // Раньше ли эта запись, чем other.
    bool is_earlier_than(const LogRecord& other) const;

private:
    Timestamp time_;
    Level level_;
    std::string module_;   // строки управляют памятью сами: malloc и free не нужны
    std::string text_;
};
