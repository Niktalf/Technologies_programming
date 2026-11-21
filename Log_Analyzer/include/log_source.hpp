#pragma once

#include <cstddef>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include "file_reader.hpp"
#include "log_book.hpp"
#include "log_record.hpp"

// Практика 12: источник записей.
//
// LogSource — абстрактный класс: next и describe чисто виртуальные, объекта
// «просто источника» создать нельзя. Кто бы ни давал записи — текстовый
// файл, двоичный, несколько файлов подряд, генератор, — потребитель
// получает их одним и тем же вызовом next и не знает, откуда они.
//
//   LogSource
//   ├── BookSource         записи уже загруженного журнала
//   ├── TextFileSource     текстовый журнал, строка за строкой
//   ├── BinaryFileSource   двоичный журнал, запись за записью
//   ├── MultiFileSource    несколько источников подряд
//   └── RandomSource       случайные записи, как у команды gen
class LogSource {
public:
    // Виртуальный деструктор: источники удаляются через
    // std::unique_ptr<LogSource>. Без virtual delete вызвал бы только
    // ~LogSource: деструктор поля reader_ текстового источника не выполнился
    // бы, и файл остался бы открытым. Стандарт называет такое удаление
    // неопределённым поведением.
    virtual ~LogSource() = default;

    // Следующая запись или пустой указатель, если записи кончились.
    // Испорченные строки текстового журнала источник пропускает сам.
    virtual std::unique_ptr<LogRecord> next() = 0;

    // Откуда записи — одной строкой, для итога отчёта.
    virtual std::string describe() const = 0;

protected:
    LogSource() = default;
};

// Записи журнала, уже загруженного в память. Журналом источник не владеет:
// это ассоциация, а не композиция. Журнал должен жить дольше источника.
class BookSource : public LogSource {
public:
    explicit BookSource(const LogBook& book)
        : book_(book) {
    }

    std::unique_ptr<LogRecord> next() override;
    std::string describe() const override;

private:
    const LogBook& book_;
    long index_ = 0;   // номер следующей записи
};

// Текстовый журнал. Файл открывается в конструкторе (не открылся —
// LogError) и закрывается деструктором поля reader_.
class TextFileSource : public LogSource {
public:
    explicit TextFileSource(const std::string& path);

    std::unique_ptr<LogRecord> next() override;
    std::string describe() const override;

private:
    FileReader reader_;
    long lines_ = 0;
    long broken_ = 0;
};

// Двоичный журнал (формат практики 8). Заголовок проверяется в конструкторе,
// записи читаются по одной. Файл оборван посреди записей — LogError из next.
class BinaryFileSource : public LogSource {
public:
    explicit BinaryFileSource(const std::string& path);

    std::unique_ptr<LogRecord> next() override;
    std::string describe() const override;

private:
    std::string path_;
    std::ifstream file_;
    unsigned long long count_ = 0;   // записей по заголовку
    unsigned long long read_ = 0;    // из них уже прочитано
};

// Несколько источников подряд: сначала все записи первого, потом второго.
//
// Наследование или композиция? MultiFileSource является источником — поэтому
// наследует LogSource: его можно отдать отчёту вместо любого другого. И он
// содержит источники — поэтому хранит их полем: это композиция. Наследовать
// его от TextFileSource было бы ошибкой: он не один файл, а несколько,
// и отношение «является» не выполняется. Так же устроен паттерн
// «Компоновщик», который встретится на практике 30.
class MultiFileSource : public LogSource {
public:
    // Забирает источник себе. Порядок добавления — порядок записей.
    void add(std::unique_ptr<LogSource> source);

    std::unique_ptr<LogRecord> next() override;
    std::string describe() const override;

private:
    std::vector<std::unique_ptr<LogSource>> sources_;
    std::size_t current_ = 0;   // номер источника, из которого читаем
};

// Случайные записи: count штук с зерном seed, те же, что пишет gen.
class RandomSource : public LogSource {
public:
    RandomSource(long count, unsigned int seed);

    std::unique_ptr<LogRecord> next() override;
    std::string describe() const override;

private:
    long count_;
    unsigned int seed_;
    long produced_ = 0;
};

// Источник для файла: двоичный, если файл начинается с сигнатуры LOGB,
// иначе текстовый. Не открылся — LogError.
std::unique_ptr<LogSource> open_file_source(const std::string& path);
