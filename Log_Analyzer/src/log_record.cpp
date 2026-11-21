#include "log_record.hpp"

#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <utility>

// Проверки — в конструкторе, а не у вызывающего: мест, где создаются
// записи, несколько (текстовый журнал, двоичный, генератор, опыты), а
// конструктор один. Забыть проверку в одном из мест теперь нельзя.
//
// Строки приняты по значению и перемещены в поля: вызывающий, которому
// строка больше не нужна, отдаёт её без копирования.
LogRecord::LogRecord(Timestamp time, Level level, std::string module, std::string text)
    : time_(time), level_(level), module_(std::move(module)), text_(std::move(text)) {
    if (!time_.valid()) {
        throw std::invalid_argument("invalid time");
    }
    // enum class не даст случайно записать число в уровень, но static_cast
    // даст: static_cast<Level>(7). Такой уровень не напечатать и не посчитать.
    int number = static_cast<int>(level_);
    if (number < 0 || number >= LEVEL_COUNT) {
        throw std::invalid_argument("unknown level");
    }
    if (module_.empty()) {
        throw std::invalid_argument("empty module");
    }
}

void LogRecord::print() const {
    std::cout << time_.text() << "  " << std::left << std::setw(5) << level_name(level_)
              << "  " << std::setw(8) << module_ << std::right << "  " << text_ << '\n';
}

// Явный this. Метод вызывается так: a.is_earlier_than(b). Компилятор
// превращает это в вызов обычной функции, которой первым, скрытым
// параметром передан адрес a — это и есть this. Внутри this->time_ —
// поле объекта a, а other.time_ — поле объекта b. Обычно this-> не пишут,
// и time_ означает то же самое; здесь он написан, чтобы было видно,
// чьё это поле.
bool LogRecord::is_earlier_than(const LogRecord& other) const {
    return this->time_.is_before(other.time_);
}
