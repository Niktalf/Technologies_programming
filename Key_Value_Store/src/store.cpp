#include "store.hpp"

#include <iostream>

bool Store::trace_ = false;

// Список инициализации: journal_ создаётся до входа в тело конструктора.
// Если его конструктор возбудит исключение, тело Store не выполнится,
// и объекта Store не будет вовсе.
Store::Store(const std::string& path)
    : journal_(path) {
    // Журнал отдаёт все целые записи по порядку и сам отрезает повреждённый
    // хвост. Если он возбудит исключение, объекта Store не будет, но уже
    // созданное поле journal_ разрушится своим деструктором — файл всё
    // равно закроется.
    std::vector<Record> records;
    broken_ = journal_.recover(records);
    // Таблица собирается применением записей по порядку — это и есть
    // первая половина инварианта.
    for (const Record& record : records) {
        apply_in_memory(record);
        ++records_;
    }
    if (trace_) {
        std::cout << "  [Store] opened " << path << ", keys " << data_.size() << '\n';
    }
}

Store::~Store() {
    // Закрывать журнал здесь не нужно: после тела этого деструктора
    // автоматически вызовется деструктор поля journal_.
    if (trace_ && journal_.is_open()) {
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
    journal_.append(record);
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
