#pragma once

#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

class Store;
enum class KvStatus;

// Запись журнала: тип операции, ключ и значение (практика 2).
//
// Запись умеет превратить себя в байты для журнала и прочитать себя
// из файла. Формат тот же, что в версии на C: заголовок из 8 байт —
// флаги, длина ключа (2 байта), длина значения (4 байта), контрольная
// сумма, — затем байты ключа и значения. Поэтому журнал, записанный
// программой на C, читается этой версией, и наоборот.
class Record {
public:
    enum class Operation { put, remove };

    // Чем кончилась попытка прочитать запись из файла.
    enum class ReadResult {
        record,   // целая запись с верной контрольной суммой
        end,      // файл кончился ровно на границе записи
        broken    // оборванная или испорченная запись: дальше пригодных данных нет
    };

    static constexpr std::size_t HEADER_SIZE = 8;
    static constexpr std::size_t KEY_MAX = 255;
    static constexpr std::size_t VALUE_MAX = 4096;

    // Флаги в первом байте заголовка.
    static constexpr std::uint8_t FLAG_PUT = 1u << 0;
    static constexpr std::uint8_t FLAG_TOMBSTONE = 1u << 1;   // надгробие: запись об удалении
    static constexpr std::uint8_t FLAG_COMPRESSED = 1u << 2;  // на будущее

    // Конструкторов пока нет (они — на практике 10), поэтому запись
    // создают статические методы с говорящими именами: Record::put("a", "1").
    static Record put(const std::string& key, const std::string& value);
    static Record remove(const std::string& key);

    Operation operation() const { return operation_; }
    const std::string& key() const { return key_; }
    const std::string& value() const { return value_; }

    // Ключ от 1 до 255 байт, значение до 4096 байт.
    bool valid() const;

    // Применяет операцию к хранилищу: то же, что Store::apply.
    KvStatus apply_to(Store& store, bool& existed) const;

    // Запись в байтах: заголовок с контрольной суммой, ключ, значение.
    std::vector<std::uint8_t> encode() const;

    // Читает следующую запись из файла. Длины из заголовка проверяются
    // до чтения по ним: испорченная длина не заставит читать мегабайты.
    static ReadResult read(std::FILE* file, Record& out);

    // Сходится ли запись в байтах: длина и контрольная сумма.
    static bool check(const std::vector<std::uint8_t>& bytes);

    // Контрольная сумма: по всем байтам, байт самой суммы считается нулём.
    static std::uint8_t checksum(const std::uint8_t* bytes, std::size_t size);

    // Флаги словами: «Flags: 0x01 (put)».
    static void print_flags(std::uint8_t flags);

private:
    Operation operation_ = Operation::put;
    std::string key_;
    std::string value_;   // у удаления значение пустое
};
