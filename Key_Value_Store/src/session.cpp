#include "session.hpp"

Session::Session(const std::string& first, const std::string& second)
    : paths_{ first, second } {
    for (int i = 0; i < STORES; ++i) {
        // Движок выбирает make_engine — единственное место, где это решается.
        stores_[i] = make_engine(paths_[i]);
    }
}

void Session::close_current() {
    stores_[current_].reset();   // движок удаляется, журнал закрывается
}

void Session::open_current(const std::string& location) {
    // Сначала новый движок, потом присваивание. Если make_engine бросит
    // исключение, присваивания не будет, и текущее останется прежним.
    stores_[current_] = make_engine(location);
    paths_[current_] = location;
}

bool Session::select(int number) {
    if (number < 1 || number > STORES) {
        return false;
    }
    current_ = number - 1;
    return true;
}
