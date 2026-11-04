#include "binary_log.hpp"

#include <cstring>
#include <fstream>
#include <stdexcept>
#include <utility>

namespace {

// Двоичный формат практики 8: сигнатура, версия, число записей, затем
// записи. Числа — младшим байтом вперёд, чтобы файл одинаково читался
// на любой машине.
const char BINARY_SIGNATURE[4] = { 'L', 'O', 'G', 'B' };
constexpr unsigned long long BINARY_VERSION = 1;
constexpr unsigned long long BINARY_TEXT_LIMIT = 65536;
constexpr unsigned long long BINARY_COUNT_LIMIT = 50000000;
constexpr unsigned long long BINARY_MODULE_LIMIT = 31;

// Пишет число size байтами, младшим вперёд.
void write_number(std::ostream& file, unsigned long long value, int size) {
    for (int i = 0; i < size; ++i) {
        file.put(static_cast<char>((value >> (8 * i)) & 0xFFu));
    }
}

// Читает число из size байтов, младшим вперёд. false, если файл кончился.
bool read_number(std::istream& file, unsigned long long& out, int size) {
    unsigned char bytes[8];
    if (!file.read(reinterpret_cast<char*>(bytes), size)) {
        return false;
    }
    out = 0;
    for (int i = 0; i < size; ++i) {
        out |= static_cast<unsigned long long>(bytes[i]) << (8 * i);
    }
    return true;
}

// Читает length байтов в строку. false, если файл кончился раньше.
bool read_text(std::istream& file, std::string& out, unsigned long long length) {
    out.assign(static_cast<std::size_t>(length), '\0');
    if (length == 0) {
        return true;
    }
    return static_cast<bool>(file.read(&out[0], static_cast<std::streamsize>(length)));
}

} // namespace

LogStatus BinaryLog::write(const LogBook& book, const std::string& path) {
    // Двоичный режим обязателен: в текстовом байты, совпадающие с кодом
    // перевода строки, на части систем портятся.
    std::ofstream file(path, std::ios::binary);
    if (!file) {
        return LogStatus::cannot_open;
    }
    file.write(BINARY_SIGNATURE, 4);
    write_number(file, BINARY_VERSION, 2);
    write_number(file, static_cast<unsigned long long>(book.count()), 8);
    for (long i = 0; i < book.count() && file; ++i) {
        const LogRecord& r = book.at(i);
        write_number(file, static_cast<unsigned long long>(r.time().value()), 8);
        write_number(file, static_cast<unsigned long long>(r.level()), 1);
        write_number(file, r.module().size(), 1);
        file.write(r.module().data(), static_cast<std::streamsize>(r.module().size()));
        write_number(file, r.text().size(), 4);
        file.write(r.text().data(), static_cast<std::streamsize>(r.text().size()));
    }
    // Часть ошибок записи видна только при закрытии файла.
    file.close();
    return file ? LogStatus::ok : LogStatus::write_failed;
}

// Длинам из файла не доверяем: каждая проверяется до чтения по ней.
std::unique_ptr<LogRecord> BinaryLog::read_record(std::istream& file) {
    unsigned long long time = 0;
    unsigned long long level = 0;
    unsigned long long module_length = 0;
    if (!read_number(file, time, 8) || !read_number(file, level, 1)
        || !read_number(file, module_length, 1)) {
        return nullptr;
    }
    if (level >= LEVEL_COUNT || module_length == 0 || module_length > BINARY_MODULE_LIMIT) {
        return nullptr;
    }
    std::string module;
    unsigned long long text_length = 0;
    if (!read_text(file, module, module_length) || !read_number(file, text_length, 4)
        || text_length > BINARY_TEXT_LIMIT) {
        return nullptr;
    }
    std::string text;
    if (!read_text(file, text, text_length)) {
        return nullptr;
    }
    // Время вне разумных пределов, неизвестный уровень, пустой модуль —
    // конструктор записи откажет, и это значит, что файл испорчен.
    Timestamp moment = Timestamp::from_value(static_cast<long long>(time));
    try {
        return std::make_unique<LogRecord>(moment, static_cast<Level>(level), std::move(module),
                                           std::move(text));
    } catch (const std::invalid_argument&) {
        return nullptr;
    }
}

bool BinaryLog::is_binary(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    char signature[4];
    return file.read(signature, 4) && std::memcmp(signature, BINARY_SIGNATURE, 4) == 0;
}

unsigned long long BinaryLog::read_header(std::istream& file, const std::string& path) {
    char signature[4];
    if (!file.read(signature, 4) || std::memcmp(signature, BINARY_SIGNATURE, 4) != 0) {
        throw LogError(LogStatus::bad_format, path);   // чужой файл отвергается сразу
    }
    unsigned long long version = 0;
    unsigned long long count = 0;
    if (!read_number(file, version, 2)) {
        throw LogError(LogStatus::corrupt, path);
    }
    if (version != BINARY_VERSION) {
        throw LogError(LogStatus::bad_format, path);
    }
    // Число записей проверяется до того, как под него что-то выделяется.
    if (!read_number(file, count, 8) || count > BINARY_COUNT_LIMIT) {
        throw LogError(LogStatus::corrupt, path);
    }
    return count;
}

LogBook BinaryLog::read(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        throw LogError(LogStatus::cannot_open, path);
    }
    unsigned long long count = read_header(file, path);
    // Записи читаются в новый вектор. Если файл оборван, исключение вылетит
    // из середины цикла: новый журнал и поток разрушатся сами, а журнал
    // вызывающего не изменится — до присваивания дело не дойдёт.
    std::vector<LogRecord> records;
    for (unsigned long long i = 0; i < count; ++i) {
        std::unique_ptr<LogRecord> record = read_record(file);
        if (!record) {
            throw LogError(LogStatus::corrupt, path);
        }
        records.push_back(std::move(*record));
    }
    // В двоичном файле нет испорченных строк: каждая запись — строка.
    // Возврат по значению: векторы не копируются.
    return LogBook(path, std::move(records), static_cast<long>(count), 0);
}
