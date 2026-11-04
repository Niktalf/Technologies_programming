#pragma once

#include <string>
#include <vector>

#include "level.hpp"
#include "log_record.hpp"

// Журнал — класс, хранящий записи.
//
// Инвариант:
//   * count() + broken() <= lines(): каждая запись и каждая испорченная
//     строка пришли из своей строки файла;
//   * если sorted(), записи упорядочены по времени — на этом держится
//     двоичный поиск в search_not_before;
//   * если не loaded(), журнал пуст: нет записей, имени файла и строк.
// Каждая запись верна по инварианту LogRecord — это обеспечивает её
// конструктор, и журнал ничего в записях не перепроверяет.
//
// Записи журнал отдаёт только по константной ссылке: at(i). Метод,
// отдающий изменяемую ссылку на records_, отменил бы весь инвариант:
// вызывающий мог бы очистить вектор, переставить записи в отсортированном
// журнале — и sorted() врал бы, а поиск находил бы не то.
//
// Практика 11: журнал больше не читает файлы. На практике 10 в нём был
// разбор строк текстового журнала и чтение и запись двоичного. Теперь это
// классы LogParser и BinaryLog, а журнал только хранит записи и даёт к ним
// доступ. Появится третий формат — изменится не журнал, а появится ещё
// один класс рядом с ними.
//
// Практика 10: журнал получает содержимое через конструктор. Отдельного
// load больше нет: объект либо создан и наполнен, либо не создан вовсе.
// Сам журнал ничем не владеет вручную — только векторами и строками, —
// поэтому особых методов ему не нужно: копирование и перемещение,
// созданные компилятором, копируют и перемещают векторы. Это правило нуля.
//
// Массив записей теперь std::vector: он сам растёт и сам освобождает
// память. Удвоение ёмкости, временный указатель для realloc, освобождение
// текстов перед массивом — всё, что на практике 7 писалось руками, —
// спрятано внутри стандартного класса.
class LogBook {
public:
    // Сколько записей печатают head, tail и выборки.
    static constexpr int SHOW = 10;

    // Пустой журнал: ничего не загружено.
    LogBook() = default;

    // Журнал из готовых записей файла path: lines строк, из них broken
    // испорченных. Если числа не сходятся с инвариантом —
    // std::invalid_argument. Записи принимаются по значению и перемещаются
    // внутрь: тот, кто их собрал, отдаёт вектор без копирования.
    LogBook(std::string path, std::vector<LogRecord> records, long lines, long broken);

    bool loaded() const { return loaded_; }
    bool sorted() const { return sorted_; }
    const std::string& path() const { return path_; }
    long lines() const { return lines_; }

    long count() const { return static_cast<long>(records_.size()); }

    // Запись по номеру, только для чтения. Номер должен быть меньше count():
    // проверять его — дело вызывающего, как и при обращении к массиву в C.
    const LogRecord& at(long index) const { return records_[static_cast<std::size_t>(index)]; }

    // Сводка после загрузки: файл, строки, записи, испорченные строки.
    void print_load_report() const;

    // Краткая сводка: то же плюс число записей каждого уровня, первая
    // и последняя по времени запись.
    void print_summary() const;

    // Первые и последние SHOW записей, прошедших фильтр.
    void print_head(const LevelFilter& filter) const;
    void print_tail(const LevelFilter& filter) const;
    long count_matching(const LevelFilter& filter) const;

    // Выборки: печатают первые SHOW записей, прошедших фильтр и условие,
    // и возвращают, сколько подходящих всего. Условие — подстрока в тексте
    // без учёта регистра, имя модуля или интервал времени (обе границы
    // включительно).
    long print_containing(const std::string& word, const LevelFilter& filter) const;
    long print_module(const std::string& module, const LevelFilter& filter) const;
    long print_between(Timestamp from, Timestamp to, const LevelFilter& filter) const;

    // Самое раннее и самое позднее время. false у пустого журнала.
    bool time_range(Timestamp& first, Timestamp& last) const;

    // Сортировка самих записей по времени слиянием, как на практике 4.
    // Возвращает наибольшую глубину рекурсии.
    int sort_by_time();

    // Номер первой записи не раньше moment двоичным поиском. Журнал должен
    // быть отсортирован. Если таких записей нет, возвращает count().
    long search_not_before(Timestamp moment) const;

private:
    // Вид выборки. Нужен, чтобы три выборки обошлись одним циклом.
    enum class Selection { text, module, interval };

    // Единственный цикл выборки. word — образец или имя модуля, from и to —
    // границы интервала; что из них нужно, решает selection.
    long print_matching(Selection selection, const std::string& word, Timestamp from,
                        Timestamp to, const LevelFilter& filter) const;

    void merge_sort(std::size_t left, std::size_t right, int depth, int& max_depth);
    void merge(std::size_t left, std::size_t middle, std::size_t right);
    long lower_bound(long low, long high, Timestamp moment) const;

    std::vector<LogRecord> records_;
    std::vector<LogRecord> buffer_;   // временный массив для слияния
    std::string path_;
    long lines_ = 0;
    long broken_ = 0;
    bool loaded_ = false;
    bool sorted_ = false;
};
