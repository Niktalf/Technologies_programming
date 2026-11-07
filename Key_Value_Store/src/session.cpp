#include "session.hpp"

Session::Session(const std::string& first, const std::string& second)
    : paths_{ first, second } {
    for (int i = 0; i < STORES; ++i) {
        // Временный открытый Store перемещается на место пустого.
        stores_[i] = Store(paths_[i]);
    }
}

void Session::close_current() {
    // Перемещающее присваивание пустого хранилища: поле journal_ прежнего
    // закрывает свой файл, таблица освобождается.
    stores_[current_] = Store();
}

void Session::open_current(const std::string& path) {
    // Сначала новое хранилище, потом присваивание. Если конструктор бросит
    // исключение, присваивания не будет, и текущее останется прежним.
    stores_[current_] = Store(path);
    paths_[current_] = path;
}

bool Session::select(int number) {
    if (number < 1 || number > STORES) {
        return false;
    }
    current_ = number - 1;
    return true;
}
