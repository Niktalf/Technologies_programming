#include "map.hpp"

#include <fstream>
#include <iostream>

#include "hero.hpp"
#include "monster_list.hpp"

// Метод вне класса записывается с именем класса: Map::reset. Без Map::
// это была бы свободная функция reset, и компоновщик не нашёл бы метод.
bool Map::reset(int width, int height) {
    if (width < MIN_SIDE || height < MIN_SIDE || width > MAX_WIDTH || height > MAX_HEIGHT) {
        return false;
    }
    // Внутри метода поля доступны просто по имени: width_ — это поле того
    // объекта, через который вызван метод. Компилятор читает его как this->width_.
    width_ = width;
    height_ = height;
    start_x_ = 1;
    start_y_ = 1;
    for (int y = 0; y < height_; ++y) {
        for (int x = 0; x < width_; ++x) {
            bool border = x == 0 || y == 0 || x == width_ - 1 || y == height_ - 1;
            cells_[y][x] = border ? WALL : FLOOR;
        }
    }
    return true;
}

LoadStatus Map::load(const std::string& path, int& bad_line, MonsterList& monsters) {
    bad_line = 0;
    std::ifstream file(path);
    if (!file) {
        return LoadStatus::not_found;
    }
    // Уровень собирается во временных карте и списке монстров. Если файл
    // испорчен, текущие остаются какими были — как в level_load версии на C.
    Map loaded;
    MonsterList loaded_monsters;
    LoadStatus status = loaded.read_rows(file, bad_line);
    if (status == LoadStatus::ok) {
        status = loaded.find_hero(bad_line, loaded_monsters);
    }
    if (status != LoadStatus::ok) {
        return status;
    }
    // Явный this. Метод вызван через какой-то объект — map.load(...), —
    // и this хранит адрес этого объекта. *this — сам объект. Присваивание
    // копирует в него все поля временной карты, включая массив клеток
    // целиком: массив — часть объекта, а не указатель на память где-то ещё.
    // Файл закроется сам, когда переменная file выйдет из области видимости.
    *this = loaded;
    monsters = loaded_monsters;   // вектор внутри списка копируется со всеми монстрами
    return LoadStatus::ok;
}

// Читает строки файла и проверяет форму: размеры, одинаковую длину строк.
LoadStatus Map::read_rows(std::istream& file, int& bad_line) {
    width_ = 0;
    height_ = 0;
    LoadStatus status = LoadStatus::ok;
    int number = 0;
    bool blank_seen = false;
    std::string line;
    while (status == LoadStatus::ok && std::getline(file, line)) {
        ++number;
        // getline убирает \n, а \r от концов строк Windows остаётся.
        while (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        int length = static_cast<int>(line.size());
        if (length == 0) {
            // Пустые строки в конце файла — обычное дело, их пропускаем.
            // А вот строка карты после пустой — уже ошибка формата.
            blank_seen = true;
        } else if (blank_seen) {
            status = LoadStatus::bad_format;
        } else if (length > MAX_WIDTH || height_ == MAX_HEIGHT) {
            status = LoadStatus::bad_size;
        } else if (height_ > 0 && length != width_) {
            status = LoadStatus::bad_format;   // строки разной длины
        } else {
            width_ = length;
            line.copy(cells_[height_], static_cast<std::size_t>(length));
            ++height_;
        }
    }
    // Ошибка чтения отличается от конца файла только состоянием потока.
    if (status == LoadStatus::ok && file.bad()) {
        status = LoadStatus::bad_format;
    }
    if (status != LoadStatus::ok) {
        bad_line = number;
        return status;
    }
    // Пустой файл и слишком маленькая карта — недопустимый размер.
    if (height_ < MIN_SIDE || width_ < MIN_SIDE) {
        return LoadStatus::bad_size;
    }
    return LoadStatus::ok;
}

// Ищет героя (он должен быть ровно один) и проверяет, что в карте нет
// чужих символов. Монстров переносит в список, а клетки героя и монстров
// заменяет полом.
LoadStatus Map::find_hero(int& bad_line, MonsterList& monsters) {
    int heroes = 0;
    for (int y = 0; y < height_; ++y) {
        for (int x = 0; x < width_; ++x) {
            char c = cells_[y][x];
            if (c == '@') {
                ++heroes;
                start_x_ = x;
                start_y_ = y;
                cells_[y][x] = FLOOR;
            } else if (c == 'M') {
                monsters.add(x, y);
                cells_[y][x] = FLOOR;
            } else if (!is_tile(c)) {
                bad_line = y + 1;
                return LoadStatus::bad_format;   // чужой символ
            }
        }
    }
    if (heroes != 1) {
        return LoadStatus::bad_format;
    }
    return LoadStatus::ok;
}

bool Map::is_tile(char c) {
    return c == WALL || c == FLOOR || c == GOLD || c == EXIT;
}

bool Map::inside(int x, int y) const {
    return x >= 0 && y >= 0 && x < width_ && y < height_;
}

char Map::tile(int x, int y) const {
    return inside(x, y) ? cells_[y][x] : WALL;
}

void Map::set_tile(int x, int y, char value) {
    if (inside(x, y)) {
        cells_[y][x] = value;
    }
}

bool Map::walkable(int x, int y) const {
    return tile(x, y) != WALL;   // tile сам проверяет границы
}

void Map::draw(const Hero* hero, const MonsterList* monsters) const {
    for (int y = 0; y < height_; ++y) {
        for (int x = 0; x < width_; ++x) {
            char shown = cells_[y][x];
            if (monsters != nullptr && monsters->find(x, y) >= 0) {
                shown = 'M';
            }
            if (hero != nullptr && hero->x() == x && hero->y() == y) {
                shown = '@';
            }
            std::cout << shown;
        }
        std::cout << '\n';
    }
}

const char* Map::status_text(LoadStatus status) {
    switch (status) {
    case LoadStatus::ok:
        return "success";
    case LoadStatus::not_found:
        return "file not found";
    case LoadStatus::bad_format:
        return "corrupted format";
    case LoadStatus::bad_size:
        return "invalid size";
    }
    return "unknown error";
}
