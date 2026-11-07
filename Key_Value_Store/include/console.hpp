#pragma once

#include <string>
#include <vector>

#include "session.hpp"

// Консольная программа: разбирает команды и вызывает методы хранилищ.
// Два хранилища — поле консоли, поэтому методам не нужно передавать их
// параметром, как функциям cli_execute(session, ...) в версии на C.
class Console {
public:
    // Результат одной команды. Скрипту мало знать «продолжать или нет»:
    // ему нужно отличить ошибку, чтобы сообщить номер строки. «Ключ
    // не найден» — это ответ, а не ошибка.
    enum class Result { quit, ok, error };

    // Открывает два хранилища. При ошибке ничего не остаётся открытым.
    KvStatus open(const std::string& first, const std::string& second);

    // Закрывает оба хранилища. Вызывается перед выходом из программы.
    void close();

    // Выполняет одну команду, заданную словами.
    Result execute(const std::vector<std::string>& words);

    // Интерактивный цикл: читает строки, пока не будет quit или конца ввода.
    void run_interactive();

private:
    Result execute_on_store(const std::vector<std::string>& words);
    Result report(KvStatus status) const;

    Result command_put(const std::vector<std::string>& words);
    Result command_get(const std::vector<std::string>& words);
    Result command_del(const std::vector<std::string>& words);
    Result command_list();
    Result command_sorted();
    Result command_range(const std::vector<std::string>& words);
    Result command_stats();
    Result command_copy(const std::vector<std::string>& words);
    Result command_close();
    Result command_use(const std::vector<std::string>& words);
    Result command_which();
    Result command_open(const std::vector<std::string>& words);
    Result command_script(const std::vector<std::string>& words);
    static Result command_genscript(const std::vector<std::string>& words);
    static Result command_record(const std::vector<std::string>& words);
    static void print_help();

    Session session_;
};
