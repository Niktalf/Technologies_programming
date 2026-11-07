#include "session.hpp"

KvStatus Session::open(const std::string& first, const std::string& second) {
    paths_[0] = first;
    paths_[1] = second;
    current_ = 0;
    KvStatus status = KvStatus::ok;
    for (int i = 0; i < STORES && status == KvStatus::ok; ++i) {
        status = stores_[i].open(paths_[i]);
    }
    if (status != KvStatus::ok) {
        close();   // что успели открыть — закрываем
    }
    return status;
}

void Session::close() {
    for (Store& store : stores_) {
        store.close();   // закрыть закрытое безопасно
    }
}

KvStatus Session::open_current(const std::string& path) {
    paths_[current_] = path;
    return stores_[current_].open(path);   // open сам закрывает прежний журнал
}

bool Session::select(int number) {
    if (number < 1 || number > STORES) {
        return false;
    }
    current_ = number - 1;
    return true;
}
