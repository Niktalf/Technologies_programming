#include "kvstore.hpp"

#include "engine.hpp"

const char* status_text(KvStatus status) {
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

KvError::KvError(KvStatus status, const std::string& path)
    : std::runtime_error(std::string(status_text(status)) + ": " + path), status_(status) {
}

// Практика 12: движок выбирает make_engine по имени базы. Если журнал
// не открылся, KvError уйдёт дальше — объекта KvStore нет.
KvStore::KvStore(const std::string& path)
    : engine_(make_engine(path)) {
}

// Здесь Engine — полный тип: деструктор unique_ptr удалит движок через
// виртуальный деструктор, а журнальный движок закроет журнал.
KvStore::~KvStore() = default;
KvStore::KvStore(KvStore&& other) noexcept = default;
KvStore& KvStore::operator=(KvStore&& other) noexcept = default;

KvStatus KvStore::put(const std::string& key, const std::string& value) {
    if (!engine_) {
        return KvStatus::bad_argument;   // объект после перемещения
    }
    bool existed = false;
    return engine_->put(key, value, existed);
}

KvStatus KvStore::get(const std::string& key, std::string& value) const {
    if (!engine_) {
        return KvStatus::bad_argument;
    }
    return engine_->get(key, value);
}

KvStatus KvStore::remove(const std::string& key) {
    if (!engine_) {
        return KvStatus::bad_argument;
    }
    return engine_->remove(key);
}

std::size_t KvStore::size() const {
    return engine_ ? engine_->size() : 0;
}

std::vector<std::pair<std::string, std::string>> KvStore::pairs() const {
    if (!engine_) {
        return {};
    }
    return engine_->pairs();
}

bool KvStore::repaired() const {
    return engine_ && engine_->repaired();
}
