#pragma once

#include <istream>
#include <string>
#include <vector>

#include "level.hpp"
#include "log_record.hpp"
#include "status.hpp"

// Журнал — класс, хранящий записи.
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

    // Читает текстовый журнал. Файл открывает и закрывает FileReader.
    // Если файл не открылся или не читается — LogError, и журнала нет.
    explicit LogBook(const std::string& path);

    // Двоичный журнал (практика 8): тот же формат, что у версии на C,
    // файлы обеих версий взаимозаменяемы. Чтение возвращает новый журнал
    // по значению; ошибка в файле — LogError.
    LogStatus save_binary(const std::string& path) const;
    static LogBook from_binary(const std::string& path);

    // Для опыта с исключением: читает файл, как конструктор, но на строке
    // номер fail_at возбуждает исключение. Файл всё равно закрывается.
    static void read_until_failure(const std::string& path, long fail_at);

    bool loaded() const { return loaded_; }
    bool sorted() const { return sorted_; }
    const std::string& path() const { return path_; }
    long lines() const { return lines_; }

    long count() const { return static_cast<long>(records_.size()); }

    // Запись по номеру. Номер должен быть меньше count(): проверять его —
    // дело вызывающего, как и при обращении к массиву в C.
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
    static LogStatus read_binary_record(std::istream& file, LogRecord& record);

    std::vector<LogRecord> records_;
    std::vector<LogRecord> buffer_;   // временный массив для слияния
    std::string path_;
    long lines_ = 0;
    long broken_ = 0;
    bool loaded_ = false;
    bool sorted_ = false;
};
