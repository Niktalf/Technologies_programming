#pragma once

#include <memory>
#include <string>
#include <vector>

#include "level.hpp"
#include "log_record.hpp"
#include "log_source.hpp"
#include "record_filter.hpp"

// Практика 12: отчёт о записях.
//
// LogReport — абстрактный класс: отчёт накапливает записи по одной
// (accept) и печатает итог (print). Откуда записи, отчёт не знает и знать
// не должен: отчёт, который сам открывает файл, не построить по источнику-
// генератору. Поэтому записи ему подаёт run_reports из любого источника.
//
//   LogReport
//   ├── LevelReport    сколько записей каждого уровня и их доля
//   ├── ModuleReport   модули по числу записей, самые частые первыми
//   └── HourReport     распределение по часам суток, столбиками
//
// На практике 11 всё это делал один класс LogStats, и чтобы добавить
// четвёртый отчёт, его пришлось бы менять. Теперь новый отчёт — новый класс.
class LogReport {
public:
    virtual ~LogReport() = default;

    // Учитывает одну запись.
    virtual void accept(const LogRecord& record) = 0;

    // Печатает итог.
    virtual void print() const = 0;

    // Название для заголовка.
    virtual const char* title() const = 0;

protected:
    LogReport() = default;
};

class LevelReport : public LogReport {
public:
    void accept(const LogRecord& record) override;
    void print() const override;
    const char* title() const override { return "records by level"; }

private:
    long total_ = 0;
    long by_level_[LEVEL_COUNT] = {};
};

class ModuleReport : public LogReport {
public:
    static constexpr int TOP = 10;   // сколько модулей показывать

    void accept(const LogRecord& record) override;
    void print() const override;
    const char* title() const override { return "top modules"; }

private:
    std::vector<std::string> modules_;   // в порядке первого появления
    std::vector<long> counts_;           // counts_[i] — записей модуля modules_[i]
};

class HourReport : public LogReport {
public:
    void accept(const LogRecord& record) override;
    void print() const override;
    const char* title() const override { return "records by hour"; }

private:
    long by_hour_[24] = {};
};

// Практика 13: сколько записей подходит под условие. Условие — значение
// типа Predicate, переданное в конструктор; отчёт хранит свою копию.
class MatchReport : public LogReport {
public:
    MatchReport(Predicate condition, std::string description);

    void accept(const LogRecord& record) override;
    void print() const override;
    const char* title() const override { return "matching records"; }

private:
    Predicate condition_;
    std::string description_;
    long matched_ = 0;
    long total_ = 0;
};

// Отчёт по имени: levels, modules, hours. Неизвестное имя — пустой указатель.
// Единственное место, где перечислены виды отчётов.
std::unique_ptr<LogReport> make_report(const std::string& name);

// Главный цикл: берёт записи из источника, пока они есть, и подаёт каждую
// всем отчётам. Возвращает, сколько записей прошло. Ни одного конкретного
// типа источника или отчёта здесь нет — только базовые классы.
long run_reports(LogSource& source, const std::vector<std::unique_ptr<LogReport>>& reports);
