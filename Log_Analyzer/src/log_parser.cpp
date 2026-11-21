#include "log_parser.hpp"

#include <iostream>
#include <stdexcept>
#include <utility>

#include "file_reader.hpp"

std::unique_ptr<LogRecord> LogParser::parse_line(const std::string& line) {
    // Ровно три поиска разделителя, а не разбиение по всем чертам подряд:
    // иначе сообщение с чертой внутри потеряет хвост.
    std::size_t first = line.find('|');
    if (first == std::string::npos) {
        return nullptr;
    }
    std::size_t second = line.find('|', first + 1);
    if (second == std::string::npos) {
        return nullptr;
    }
    std::size_t third = line.find('|', second + 1);
    if (third == std::string::npos) {
        return nullptr;
    }
    Level level = Level::info;
    if (!level_from_word(line.substr(first + 1, second - first - 1), level)) {
        return nullptr;
    }
    // Время и модуль проверяет конструктор записи: неверное время или пустой
    // модуль — std::invalid_argument. Здесь это не авария, а испорченная
    // строка, и исключение превращается в пустой указатель.
    try {
        return std::make_unique<LogRecord>(Timestamp::parse(line.substr(0, first)), level,
                                           line.substr(second + 1, third - second - 1),
                                           line.substr(third + 1));
    } catch (const std::invalid_argument&) {
        return nullptr;
    }
}

// Объект reader живёт до конца метода. Как бы метод ни завершился —
// дочитав файл или исключением, — деструктор reader закроет файл.
LogBook LogParser::read(const std::string& path) {
    FileReader reader(path);
    std::vector<LogRecord> records;
    long lines = 0;
    long broken = 0;
    std::string line;
    while (reader.read_line(line)) {
        ++lines;
        // Пустая строка — не испорченная запись, её просто пропускаем.
        if (!line.empty()) {
            std::unique_ptr<LogRecord> record = parse_line(line);
            if (record) {
                records.push_back(std::move(*record));   // запись переезжает в вектор
            } else {
                ++broken;
            }
        }
    }
    // Вектор записей перемещается в журнал, журнал возвращается
    // по значению: записи не копируются ни разу.
    return LogBook(path, std::move(records), lines, broken);
}   // здесь деструктор reader закрывает файл; журнал живёт дальше

void LogParser::read_until_failure(const std::string& path, long fail_at) {
    FileReader reader(path);
    std::string line;
    long number = 0;
    while (reader.read_line(line)) {
        ++number;
        if (number == fail_at) {
            std::cout << "  throwing an exception at line " << number << '\n';
            // Выход из функции посреди чтения. В версии на C здесь нужно
            // было бы не забыть fclose; здесь его нет — и он не нужен.
            throw std::runtime_error("error at line " + std::to_string(number));
        }
    }
}
