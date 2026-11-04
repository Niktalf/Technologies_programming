#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "level.hpp"
#include "log_book.hpp"
#include "log_source.hpp"
#include "log_view.hpp"

// Командная оболочка: журнал, фильтр по уровням, порядок просмотра
// и команды над ними. На практике 8 фильтр жил локальной переменной
// shell_run, порядок просмотра — статическими переменными модуля view,
// а функции команд получали журнал параметром. Здесь всё это — поля
// одного объекта, а команды — его закрытые методы.
//
// Инвариант: если журнал не загружен, вид view_ пуст; если загружен,
// указатели вида смотрят в записи текущего журнала. Поэтому каждая
// команда, которая заменяет или сортирует журнал, сбрасывает вид.
class Shell {
public:
    // Загружает журнал при запуске. Ошибка — LogError, как у конструктора
    // LogBook.
    void open(const std::string& path);

    // Интерактивный цикл: читает строки, пока не будет quit или конца ввода.
    void run();

private:
    // Выполняет одну команду. words[0] — её название в нижнем регистре.
    // Возвращает false, если пора завершать работу.
    bool execute(const std::vector<std::string>& words);

    bool require_log() const;
    void print_found(long found) const;

    void command_load(const std::vector<std::string>& words);
    void command_bsave(const std::vector<std::string>& words);
    void command_bload(const std::vector<std::string>& words);
    void command_gen(const std::vector<std::string>& words);
    void command_count();
    void command_order(const std::vector<std::string>& words);
    void command_view(const std::vector<std::string>& words);
    void command_span() const;
    void command_sort();
    void command_since(const std::vector<std::string>& words);
    void command_search(const std::vector<std::string>& words);
    void command_between(const std::vector<std::string>& words);
    void command_level(const std::vector<std::string>& words);
    static void command_time();
    void command_demo();
    static void try_to_break();
    void command_stress(const std::vector<std::string>& words);
    void command_stats(const std::string& name) const;
    void command_report(const std::vector<std::string>& words);
    std::unique_ptr<LogSource> make_source(const std::vector<std::string>& words,
                                           std::size_t position) const;
    static void print_help();

    LogBook book_;
    LevelFilter filter_;
    LogView view_;
};
