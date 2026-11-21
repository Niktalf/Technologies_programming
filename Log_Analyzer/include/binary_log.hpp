#pragma once

#include <istream>
#include <string>
#include <vector>

#include "log_book.hpp"
#include "log_record.hpp"
#include "status.hpp"

// Практика 11: двоичный журнал (формат практики 8).
//
// Тот же формат, что у версии на C, файлы обеих версий взаимозаменяемы.
// На практике 10 чтение и запись были методами журнала. Теперь журнал
// хранит записи, а как разложить их в байты — знает этот класс. Он видит
// журнал только через открытые методы: count() и at(i) при записи,
// конструктор LogBook при чтении.
//
// Состояния у класса нет — нет и инварианта. Поэтому методы статические.
class BinaryLog {
public:
    static LogStatus write(const LogBook& book, const std::string& path);

    // Читает журнал и возвращает его по значению. Ошибка в файле — LogError.
    static LogBook read(const std::string& path);

private:
    // Читает одну запись и добавляет её в конец records.
    static LogStatus read_record(std::istream& file, std::vector<LogRecord>& records);
};
