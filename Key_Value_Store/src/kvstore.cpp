#include "kvstore.hpp"

#include "store.hpp"

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

// std::make_unique создаёт Store в куче и сразу отдаёт его во владение
// unique_ptr. Если конструктор Store бросит KvError, памяти не останется:
// make_unique освободит её сам, и исключение уйдёт дальше — объекта KvStore нет.
KvStore::KvStore(const std::string& path)
    : store_(std::make_unique<Store>(path)) {
}

// Здесь Store — полный тип: деструктор unique_ptr удалит его, а деструктор
// Store закроет журнал.
KvStore::~KvStore() = default;
KvStore::KvStore(KvStore&& other) noexcept = default;
KvStore& KvStore::operator=(KvStore&& other) noexcept = default;

KvStatus KvStore::put(const std::string& key, const std::string& value) {
    if (!store_) {
        return KvStatus::bad_argument;   // объект после перемещения
    }
    bool existed = false;
    return store_->put(key, value, existed);
}

KvStatus KvStore::get(const std::string& key, std::string& value) const {
    if (!store_) {
        return KvStatus::bad_argument;
    }
    return store_->get(key, value);
}

KvStatus KvStore::remove(const std::string& key) {
    if (!store_) {
        return KvStatus::bad_argument;
    }
    return store_->remove(key);
}

std::size_t KvStore::size() const {
    return store_ ? store_->size() : 0;
}

std::vector<std::pair<std::string, std::string>> KvStore::pairs() const {
    if (!store_) {
        return {};
    }
    return store_->pairs();
}

bool KvStore::repaired() const {
    return store_ && store_->recovered_broken() > 0;
}
