#include "log_record.hpp"

#include <iomanip>
#include <iostream>

bool LogRecord::parse(const std::string& line) {
    // Ровно три поиска разделителя, а не разбиение по всем чертам подряд:
    // иначе сообщение с чертой внутри потеряет хвост.
    std::size_t first = line.find('|');
    if (first == std::string::npos) {
        return false;
    }
    std::size_t second = line.find('|', first + 1);
    if (second == std::string::npos) {
        return false;
    }
    std::size_t third = line.find('|', second + 1);
    if (third == std::string::npos) {
        return false;
    }
    Level level = Level::info;
    if (!level_from_word(line.substr(first + 1, second - first - 1), level)) {
        return false;
    }
    return assign(Timestamp::parse(line.substr(0, first)), level,
                  line.substr(second + 1, third - second - 1), line.substr(third + 1));
}

bool LogRecord::assign(Timestamp time, Level level, const std::string& module,
                       const std::string& text) {
    if (!time.valid() || module.empty()) {
        return false;
    }
    // Поля меняются, только когда всё проверено: неудачный разбор
    // не оставляет запись наполовину заполненной.
    time_ = time;
    level_ = level;
    module_ = module;
    text_ = text;
    return true;
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
