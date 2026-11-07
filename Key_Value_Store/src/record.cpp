#include "record.hpp"

#include <iomanip>
#include <iostream>

#include "store.hpp"

Record Record::put(const std::string& key, const std::string& value) {
    Record record;
    record.operation_ = Operation::put;   // закрытые поля доступны: метод того же класса
    record.key_ = key;
    record.value_ = value;
    return record;
}

Record Record::remove(const std::string& key) {
    Record record;
    record.operation_ = Operation::remove;
    record.key_ = key;
    return record;
}

bool Record::valid() const {
    return !key_.empty() && key_.size() <= KEY_MAX && value_.size() <= VALUE_MAX;
}

// Явный this. Вызов record.apply_to(store, existed) компилятор превращает
// в вызов функции, которой скрытым первым параметром передан адрес record —
// это и есть this. Записи нужно передать хранилищу саму себя, и *this —
// единственный способ назвать «этот объект» изнутри метода.
KvStatus Record::apply_to(Store& store, bool& existed) const {
    return store.apply(*this, existed);
}

std::uint8_t Record::checksum(const std::uint8_t* bytes, std::size_t size) {
    // Умножение на нечётное число разводит перестановки байтов: простая
    // сумма не заметила бы обмен двух байтов местами.
    std::uint8_t sum = 0;
    for (std::size_t i = 0; i < size; ++i) {
        std::uint8_t byte = i == HEADER_SIZE - 1 ? 0 : bytes[i];
        sum = static_cast<std::uint8_t>(sum * 31u + byte);
    }
    return sum;
}

std::vector<std::uint8_t> Record::encode() const {
    std::size_t key_length = key_.size();
    std::size_t value_length = value_.size();
    std::vector<std::uint8_t> bytes(HEADER_SIZE + key_length + value_length);
    // Поля раскладываются по байтам вручную, старший байт первым:
    // файл одинаково читается на любой машине.
    bytes[0] = operation_ == Operation::put ? FLAG_PUT : FLAG_TOMBSTONE;
    bytes[1] = static_cast<std::uint8_t>((key_length >> 8) & 0xFFu);
    bytes[2] = static_cast<std::uint8_t>(key_length & 0xFFu);
    bytes[3] = static_cast<std::uint8_t>((value_length >> 24) & 0xFFu);
    bytes[4] = static_cast<std::uint8_t>((value_length >> 16) & 0xFFu);
    bytes[5] = static_cast<std::uint8_t>((value_length >> 8) & 0xFFu);
    bytes[6] = static_cast<std::uint8_t>(value_length & 0xFFu);
    bytes[7] = 0;   // сумма считается по заголовку с нулевым полем суммы
    for (std::size_t i = 0; i < key_length; ++i) {
        bytes[HEADER_SIZE + i] = static_cast<std::uint8_t>(key_[i]);
    }
    for (std::size_t i = 0; i < value_length; ++i) {
        bytes[HEADER_SIZE + key_length + i] = static_cast<std::uint8_t>(value_[i]);
    }
    bytes[7] = checksum(bytes.data(), bytes.size());
    return bytes;
}

bool Record::check(const std::vector<std::uint8_t>& bytes) {
    if (bytes.size() < HEADER_SIZE) {
        return false;
    }
    // Длина из заголовка проверяется до всякой работы с данными:
    // испорченная длина опаснее испорченных данных.
    std::size_t key_length = (static_cast<std::size_t>(bytes[1]) << 8) | bytes[2];
    std::size_t value_length = (static_cast<std::size_t>(bytes[3]) << 24)
                               | (static_cast<std::size_t>(bytes[4]) << 16)
                               | (static_cast<std::size_t>(bytes[5]) << 8) | bytes[6];
    if (HEADER_SIZE + key_length + value_length != bytes.size()) {
        return false;
    }
    return checksum(bytes.data(), bytes.size()) == bytes[HEADER_SIZE - 1];
}

Record::ReadResult Record::read(std::FILE* file, Record& out) {
    std::vector<std::uint8_t> bytes(HEADER_SIZE);
    std::size_t got = std::fread(bytes.data(), 1, HEADER_SIZE, file);
    if (got == 0) {
        return ReadResult::end;
    }
    if (got != HEADER_SIZE) {
        return ReadResult::broken;   // оборван уже заголовок
    }
    std::size_t key_length = (static_cast<std::size_t>(bytes[1]) << 8) | bytes[2];
    std::size_t value_length = (static_cast<std::size_t>(bytes[3]) << 24)
                               | (static_cast<std::size_t>(bytes[4]) << 16)
                               | (static_cast<std::size_t>(bytes[5]) << 8) | bytes[6];
    if (key_length == 0 || key_length > KEY_MAX || value_length > VALUE_MAX) {
        return ReadResult::broken;
    }
    std::size_t body = key_length + value_length;
    bytes.resize(HEADER_SIZE + body);
    if (std::fread(bytes.data() + HEADER_SIZE, 1, body, file) != body || !check(bytes)) {
        return ReadResult::broken;   // запись оборвана или не сошлась сумма
    }
    const char* text = reinterpret_cast<const char*>(bytes.data()) + HEADER_SIZE;
    out.operation_ = (bytes[0] & FLAG_TOMBSTONE) != 0 ? Operation::remove : Operation::put;
    out.key_.assign(text, key_length);
    out.value_.assign(text + key_length, value_length);
    return ReadResult::record;
}

void Record::print_flags(std::uint8_t flags) {
    std::ios::fmtflags saved = std::cout.flags();
    char fill = std::cout.fill();
    std::cout << "Flags: 0x" << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
              << static_cast<int>(flags);
    std::cout.flags(saved);
    std::cout.fill(fill);
    std::cout << " (";
    bool printed = false;
    if ((flags & FLAG_PUT) != 0) {
        std::cout << "put";
        printed = true;
    }
    if ((flags & FLAG_TOMBSTONE) != 0) {
        std::cout << (printed ? ", " : "") << "tombstone";
        printed = true;
    }
    if ((flags & FLAG_COMPRESSED) != 0) {
        std::cout << (printed ? ", " : "") << "compressed";
        printed = true;
    }
    std::cout << (printed ? "" : "no") << ")\n";
}
