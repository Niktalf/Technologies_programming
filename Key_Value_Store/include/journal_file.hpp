#pragma once

#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#include "record.hpp"

// Практика 10: класс, владеющий файлом журнала.
//
// Конструктор открывает файл, деструктор сбрасывает буфер на диск
// и закрывает. Отдельных методов open и close нет: файл открыт ровно
// столько, сколько живёт объект. Деструктор объекта на стеке вызывается
// при любом выходе из области видимости, в том числе по исключению.
//
// Правило пяти: деструктор закрывает файл, копирование запрещено,
// перемещение передаёт файл другому объекту.
class JournalFile {
public:
    // Пустой объект: файлом не владеет. Так выглядит закрытое хранилище
    // и объект, из которого файл забрали перемещением.
    JournalFile() = default;

    // Открывает журнал на чтение и дозапись; если файла нет, создаёт.
    // При неудаче возбуждает KvError.
    explicit JournalFile(const std::string& path);

    // Сбрасывает буфер и закрывает файл. Исключений не возбуждает.
    ~JournalFile();

    // Копирование запрещено. Копия получила бы тот же FILE *: два
    // деструктора закрыли бы один файл дважды, а записи двух объектов
    // перемешались бы в одном буфере.
    JournalFile(const JournalFile&) = delete;
    JournalFile& operator=(const JournalFile&) = delete;

    // Перемещение: файл переходит к новому владельцу, прежний остаётся пустым.
    JournalFile(JournalFile&& other) noexcept;
    JournalFile& operator=(JournalFile&& other) noexcept;

    bool is_open() const { return file_ != nullptr; }
    const std::string& path() const { return path_; }

    // Встаёт в начало файла, чтобы перечитать журнал.
    void rewind_for_reading();

    // Читает следующую запись в out. Возвращает, чем кончилась попытка:
    // запись, конец файла или повреждённые данные.
    Record::ReadResult read_next(Record& out);

    // Отрезает всё после последней целой записи: переписывает целые записи
    // во временный файл и ставит его на место старого. Ошибка — KvError.
    void cut_after_last_record();

    // Дописывает байты записи в конец и сразу сбрасывает их на диск.
    // При ошибке — KvError.
    void append(const std::vector<std::uint8_t>& bytes);

    // Сколько журналов открыто сейчас всеми объектами — для проверки,
    // что после ста циклов открытых файлов не осталось.
    static int open_count() { return open_count_; }
    static bool trace;

private:
    // Сбрасывает буфер и закрывает свой файл, если он есть.
    void close() noexcept;

    std::FILE* file_ = nullptr;   // nullptr — объект файлом не владеет
    std::string path_;
    long good_end_ = 0;           // конец последней целой записи
    bool reading_ = false;        // последней операцией было чтение

    static int open_count_;
};
