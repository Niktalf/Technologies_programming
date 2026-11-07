#pragma once

// Результат операции, которая может не получиться. Те же причины, что
// у LogStatus в версии на C, кроме нехватки памяти: строки и векторы
// выделяют память сами, и её нехватку мы пока не обрабатываем.
enum class LogStatus { ok, cannot_open, bad_format, corrupt, bad_argument, write_failed };

// Текст для сообщения — те же слова, что в версии на C.
const char* status_text(LogStatus status);

#include <stdexcept>
#include <string>

// Практика 10: ошибка загрузки приходит исключением. LogError — наследник
// std::runtime_error: ловится и как LogError, и как любое std::exception.
// Кроме текста хранит код, чтобы оболочка напечатала то же сообщение,
// что и версия на C.
class LogError : public std::runtime_error {
public:
    LogError(LogStatus status, const std::string& path)
        : std::runtime_error(std::string(status_text(status)) + " (" + path + ")"),
          status_(status) {
    }

    LogStatus status() const { return status_; }

private:
    LogStatus status_;
};
