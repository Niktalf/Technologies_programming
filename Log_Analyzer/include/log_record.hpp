#pragma once

#include <string>

#include "level.hpp"
#include "timestamp.hpp"

// Запись журнала — класс.
//
// В версии на C запись была структурой, а функции разбора и печати —
// свободными функциями, получавшими структуру первым параметром:
// parse_line(line, &record). Здесь данные и работающие с ними функции
// объединены, а поля закрыты: снаружи их можно только прочитать
// через методы.
class LogRecord {
public:
    // Разбирает строку «2026-03-15 10:30:00|ERROR|net|текст». Первые три
    // черты делят поля, всё после третьей — текст. При неудаче возвращает
    // false и запись не меняет.
    bool parse(const std::string& line);

    // Запись из готовых полей — при чтении двоичного файла. Возвращает
    // false, если поля недопустимы; запись тогда не меняется.
    bool assign(Timestamp time, Level level, const std::string& module, const std::string& text);

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
    Level level_ = Level::info;
    std::string module_;   // строки управляют памятью сами: malloc и free не нужны
    std::string text_;
};
