#include "store.hpp"

#include <iostream>

bool Store::trace = false;

KvError::KvError(KvStatus status, const std::string& path)
    : std::runtime_error(std::string(Store::status_text(status)) + ": " + path),
      status_(status) {
}

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

// Список инициализации: journal_ создаётся до входа в тело конструктора.
// Если его конструктор возбудит исключение, тело Store не выполнится,
// и объекта Store не будет вовсе.
Store::Store(const std::string& path)
    : journal_(path) {
    // Читаем журнал от начала и собираем таблицу. Первая запись, которая
    // не читается целиком или не сходится по контрольной сумме, означает
    // конец пригодных данных: всё дальше отбрасывается.
    journal_.rewind_for_reading();
    Record record;
    Record::ReadResult result = journal_.read_next(record);
    while (result == Record::ReadResult::record) {
        apply_in_memory(record);
        ++records_;
        result = journal_.read_next(record);
    }
    // Повреждённый хвост — след аварии на середине записи. Он отрезается,
    // иначе новые записи легли бы после мусора и при следующем открытии
    // не прочитались бы. Если обрезка возбудит исключение, объекта Store
    // не будет, но уже созданное поле journal_ разрушится своим
    // деструктором — файл всё равно закроется.
    if (result == Record::ReadResult::broken) {
        ++broken_;
        journal_.cut_after_last_record();
    }
    if (trace) {
        std::cout << "  [Store] opened " << path << ", keys " << data_.size() << '\n';
    }
}

Store::~Store() {
    // Закрывать журнал здесь не нужно: после тела этого деструктора
    // автоматически вызовется деструктор поля journal_.
    if (trace && journal_.is_open()) {
        std::cout << "  [Store] closing " << journal_.path() << '\n';
    }
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
    if (!data_.insert_or_assign(record.key(), record.value()).second) {
        ++garbage_;
    }
}

KvStatus Store::apply(const Record& record, bool& existed) {
    existed = data_.count(record.key()) > 0;
    if (!journal_.is_open() || !record.valid()) {
        return KvStatus::bad_argument;
    }
    // Удаление отсутствующего ключа в журнал не пишется: менять нечего.
    if (record.operation() == Record::Operation::remove && !existed) {
        return KvStatus::not_found;
    }
    // Сначала журнал, потом память. Если запись в журнал не удалась,
    // append возбудит исключение, и таблица останется прежней.
    journal_.append(record.encode());
    ++records_;
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
