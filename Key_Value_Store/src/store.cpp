#include "store.hpp"

#include <cstdio>

const char* Store::status_text(KvStatus status) {
    switch (status) {
    case KvStatus::ok:
        return "success";
    case KvStatus::bad_argument:
        return "invalid argument";
    case KvStatus::not_found:
        return "key not found";
    case KvStatus::cannot_open:
        return "cannot open file";
    case KvStatus::io_error:
        return "journal read or write error";
    }
    return "unknown error";
}

KvStatus Store::open(const std::string& path) {
    close();
    path_ = path;
    // Режим "r+b" открывает существующий файл на чтение и запись, не обрезая
    // его. Если файла нет, создаём его режимом "w+b". Двоичный режим
    // обязателен: текстовый на части систем портит байты перевода строки.
    journal_ = std::fopen(path.c_str(), "r+b");
    if (journal_ == nullptr) {
        journal_ = std::fopen(path.c_str(), "w+b");
    }
    if (journal_ == nullptr) {
        return KvStatus::cannot_open;
    }
    long good_end = 0;
    KvStatus status = recover(good_end);
    if (status == KvStatus::ok && broken_ > 0) {
        status = cut_tail(good_end);
    }
    // Дописывать будем с конца целых записей.
    if (status == KvStatus::ok && std::fseek(journal_, good_end, SEEK_SET) != 0) {
        status = KvStatus::io_error;
    }
    if (status != KvStatus::ok) {
        close();   // закрывает файл и очищает всё собранное
    }
    return status;
}

void Store::close() {
    if (journal_ != nullptr) {
        // Каждая запись уже сброшена на диск в append, поэтому терять
        // при закрытии нечего.
        std::fclose(journal_);
        journal_ = nullptr;
    }
    data_.clear();
    records_ = 0;
    garbage_ = 0;
    broken_ = 0;
}

// Дописывает запись в конец журнала и сбрасывает её на диск. Сброс
// обязателен: без него при аварии потеряется не последняя операция,
// а всё, что осело в буфере.
KvStatus Store::append(const Record& record) {
    std::vector<std::uint8_t> bytes = record.encode();
    if (std::fwrite(bytes.data(), 1, bytes.size(), journal_) != bytes.size()
        || std::fflush(journal_) != 0) {
        return KvStatus::io_error;
    }
    ++records_;
    return KvStatus::ok;
}

// Применяет запись к таблице. Побеждает последняя запись о ключе,
// прежние становятся мусором.
void Store::apply_in_memory(const Record& record) {
    if (record.operation() == Record::Operation::remove) {
        if (data_.erase(record.key()) > 0) {
            ++garbage_;   // удалённая пара стала мусором
        }
        ++garbage_;       // и сама запись об удалении тоже
        return;
    }
    // insert_or_assign: вставка и обновление — одна операция, второй пары
    // с тем же ключом не появится. second == false — ключ уже был.
    if (!data_.insert_or_assign(record.key(), record.value()).second) {
        ++garbage_;
    }
}

// Читает журнал от начала и собирает таблицу. Первая запись, которая
// не читается целиком или не сходится по контрольной сумме, означает конец
// пригодных данных: всё дальше отбрасывается.
KvStatus Store::recover(long& good_end) {
    good_end = 0;
    KvStatus status = KvStatus::ok;
    Record::ReadResult result = Record::ReadResult::record;
    while (status == KvStatus::ok && result == Record::ReadResult::record) {
        Record record;
        result = Record::read(journal_, record);
        if (result == Record::ReadResult::record) {
            apply_in_memory(record);
            ++records_;
            good_end = std::ftell(journal_);
            if (good_end < 0) {
                status = KvStatus::io_error;
            }
        } else if (result == Record::ReadResult::broken) {
            ++broken_;
        }
    }
    // Ошибка чтения выглядит для fread как конец файла: различаем их
    // через ferror, иначе сбой диска сошёл бы за короткий журнал.
    if (status == KvStatus::ok && std::ferror(journal_) != 0) {
        status = KvStatus::io_error;
    }
    return status;
}

namespace {

// Копирует первые length байт файла from в файл to. true при успехе.
bool copy_prefix(std::FILE* from, std::FILE* to, long length) {
    unsigned char chunk[4096];
    long left = length;
    bool ok = std::fseek(from, 0, SEEK_SET) == 0;
    while (ok && left > 0) {
        std::size_t want = left < static_cast<long>(sizeof chunk)
                               ? static_cast<std::size_t>(left) : sizeof chunk;
        ok = std::fread(chunk, 1, want, from) == want && std::fwrite(chunk, 1, want, to) == want;
        left -= static_cast<long>(want);
    }
    return ok;
}

} // namespace

// Отрезает от журнала повреждённый хвост: копирует целые записи во временный
// файл и ставит его на место старого. Писать дальше поверх хвоста нельзя:
// новая запись короче мусора, и остаток мусора будет снова прочитан при
// следующем открытии. В стандартной библиотеке C нет функции «обрезать
// файл», поэтому хвост отрезается переписыванием, как в версии на C.
// Случается это только после аварии.
KvStatus Store::cut_tail(long good_end) {
    std::string temp_path = path_ + ".tmp";
    std::FILE* temp = std::fopen(temp_path.c_str(), "wb");
    if (temp == nullptr) {
        return KvStatus::cannot_open;
    }
    bool copied = copy_prefix(journal_, temp, good_end);
    // Результат fclose проверяется: данные из буфера уходят на диск именно тогда.
    if (std::fclose(temp) != 0 || !copied) {
        std::remove(temp_path.c_str());
        return KvStatus::io_error;
    }
    // Старый файл закрываем и заменяем новым. На некоторых системах rename
    // не перезаписывает существующий файл, поэтому сначала remove.
    std::fclose(journal_);
    journal_ = nullptr;
    if (std::remove(path_.c_str()) != 0 || std::rename(temp_path.c_str(), path_.c_str()) != 0) {
        return KvStatus::io_error;
    }
    journal_ = std::fopen(path_.c_str(), "r+b");
    return journal_ != nullptr ? KvStatus::ok : KvStatus::cannot_open;
}

KvStatus Store::apply(const Record& record, bool& existed) {
    existed = data_.count(record.key()) > 0;
    if (journal_ == nullptr || !record.valid()) {
        return KvStatus::bad_argument;
    }
    // Удаление отсутствующего ключа в журнал не пишется: менять нечего.
    if (record.operation() == Record::Operation::remove && !existed) {
        return KvStatus::not_found;
    }
    KvStatus status = append(record);
    if (status != KvStatus::ok) {
        return status;   // в журнал не попало — память не трогаем
    }
    apply_in_memory(record);
    return KvStatus::ok;
}

KvStatus Store::put(const std::string& key, const std::string& value, bool& existed) {
    return apply(Record::put(key, value), existed);
}

KvStatus Store::remove(const std::string& key) {
    bool existed = false;
    return apply(Record::remove(key), existed);
}

KvStatus Store::get(const std::string& key, std::string& value) const {
    auto found = data_.find(key);
    if (found == data_.end()) {
        return KvStatus::not_found;
    }
    value = found->second;
    return KvStatus::ok;
}

std::vector<std::pair<std::string, std::string>> Store::pairs() const {
    std::vector<std::pair<std::string, std::string>> result;
    result.reserve(data_.size());
    for (const auto& pair : data_) {
        result.push_back(pair);
    }
    return result;
}
