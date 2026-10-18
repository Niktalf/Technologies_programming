#include "map.hpp"

#include <fstream>
#include <iostream>

#include "hero.hpp"
#include "monster_list.hpp"

// Статические поля объявлены в классе, а определяются здесь, один раз
// на всю программу.
bool Map::trace = false;
int Map::live_count_ = 0;

LevelError::LevelError(LoadStatus status, int line)
    : std::runtime_error(describe(status, line)), status_(status), line_(line) {
}

std::string LevelError::describe(LoadStatus status, int line) {
    std::string text = Map::status_text(status);
    if (line > 0) {
        text += " (line " + std::to_string(line) + ")";
    }
    return text;
}

bool Map::is_tile(char c) {
    return c == WALL || c == FLOOR || c == GOLD || c == EXIT;
}

// Правила те же, что у версии на C и у практики 9. Ошибка — LevelError.
// Строки возвращаются вектором: при исключении вектор освобождает себя сам.
std::vector<std::string> Map::read_rows(std::istream& file) {
    std::vector<std::string> rows;
    int number = 0;
    bool blank_seen = false;
    std::string line;
    while (std::getline(file, line)) {
        ++number;
        while (!line.empty() && line.back() == '\r') {
            line.pop_back();   // файл из Windows
        }
        if (line.empty()) {
            // Пустые строки в конце файла — обычное дело, их пропускаем.
            // А вот строка карты после пустой — уже ошибка формата.
            blank_seen = true;
        } else if (blank_seen) {
            throw LevelError(LoadStatus::bad_format, number);
        } else if (static_cast<int>(line.size()) > MAX_WIDTH
                   || static_cast<int>(rows.size()) == MAX_HEIGHT) {
            throw LevelError(LoadStatus::bad_size, number);
        } else if (!rows.empty() && line.size() != rows[0].size()) {
            throw LevelError(LoadStatus::bad_format, number);   // строки разной длины
        } else {
            rows.push_back(line);
        }
    }
    if (file.bad()) {
        throw LevelError(LoadStatus::bad_format, number);
    }
    // Пустой файл и слишком маленькая карта — недопустимый размер.
    if (static_cast<int>(rows.size()) < MIN_SIDE
        || static_cast<int>(rows[0].size()) < MIN_SIDE) {
        throw LevelError(LoadStatus::bad_size, 0);
    }
    return rows;
}

// Список инициализации: поля получают значения до входа в тело
// конструктора. Проверка размера стоит до new[]: если размер неверный,
// исключение вылетит, пока выделять ещё нечего.
Map::Map(int width, int height)
    : width_(width), height_(height),
      name_("room " + std::to_string(width) + "x" + std::to_string(height)) {
    if (width < MIN_SIDE || height < MIN_SIDE || width > MAX_WIDTH || height > MAX_HEIGHT) {
        throw std::invalid_argument("invalid map size");
    }
    cells_ = new char[static_cast<std::size_t>(width_) * static_cast<std::size_t>(height_)];
    clear();
    ++live_count_;
    if (trace) {
        std::cout << "  [Map] created " << name_ << '\n';
    }
}

// Загрузка устроена так, чтобы исключение могло возникнуть только
// до выделения памяти. Файл читается в std::vector<std::string>: и поток,
// и вектор, и строки освобождают себя сами, если конструктор завершится
// исключением. Память под клетки выделяется, когда файл уже прочитан
// и проверен, и дальше исключений нет.
//
// Почему это важно. Если конструктор возбуждает исключение, объект
// не считается созданным, и его деструктор НЕ вызывается. Выдели мы cells_
// через new[] в начале конструктора и возбуди LevelError на третьей строке
// файла — блок остался бы в куче навсегда: delete[] стоит в деструкторе,
// а деструктора не будет.
Map::Map(const std::string& path, MonsterList& monsters)
    : name_(path) {
    std::ifstream file(path);
    if (!file) {
        throw LevelError(LoadStatus::not_found, 0);
    }
    std::vector<std::string> rows = read_rows(file);

    // Герой должен быть ровно один, чужих символов быть не должно. Монстры
    // собираются во временный список и попадают в monsters только при успехе.
    MonsterList found;
    int heroes = 0;
    for (std::size_t y = 0; y < rows.size(); ++y) {
        for (std::size_t x = 0; x < rows[y].size(); ++x) {
            char c = rows[y][x];
            if (c == '@') {
                ++heroes;
                start_x_ = static_cast<int>(x);
                start_y_ = static_cast<int>(y);
            } else if (c == 'M') {
                found.add(static_cast<int>(x), static_cast<int>(y));
            } else if (!is_tile(c)) {
                throw LevelError(LoadStatus::bad_format, static_cast<int>(y) + 1);
            }
        }
    }
    if (heroes != 1) {
        throw LevelError(LoadStatus::bad_format, 0);
    }

    // Всё проверено — только теперь выделяем.
    height_ = static_cast<int>(rows.size());
    width_ = static_cast<int>(rows[0].size());
    cells_ = new char[static_cast<std::size_t>(width_) * static_cast<std::size_t>(height_)];
    for (int y = 0; y < height_; ++y) {
        for (int x = 0; x < width_; ++x) {
            char c = rows[static_cast<std::size_t>(y)][static_cast<std::size_t>(x)];
            cells_[index(x, y)] = c == '@' || c == 'M' ? FLOOR : c;
        }
    }
    // Вектор монстров переезжает в список вызывающего: std::move разрешает
    // забрать содержимое found, а не копировать его.
    monsters = std::move(found);
    ++live_count_;
    if (trace) {
        std::cout << "  [Map] loaded " << name_ << ", " << width_ << " x " << height_ << '\n';
    }
}   // file закрывается здесь сам: деструктор std::ifstream

Map::~Map() {
    if (cells_ != nullptr) {
        --live_count_;   // пустая карта после перемещения клетками не владеет
        if (trace) {
            std::cout << "  [Map] destroyed " << name_ << '\n';
        }
    }
    delete[] cells_;   // парная форма к new[]; delete[] от nullptr ничего не делает
}

void Map::take_from(Map& other) {
    width_ = other.width_;
    height_ = other.height_;
    start_x_ = other.start_x_;
    start_y_ = other.start_y_;
    cells_ = other.cells_;
    name_ = std::move(other.name_);
    // Прежний владелец остаётся пустым: его деструктор ничего не освободит.
    other.cells_ = nullptr;
    other.width_ = 0;
    other.height_ = 0;
}

// Перемещающий конструктор: новый объект забирает блок клеток. Новой памяти
// не выделяется, клетки не копируются — переезжает один указатель.
Map::Map(Map&& other) noexcept {
    take_from(other);
    if (trace) {
        std::cout << "  [Map] moved " << name_ << " to a new object\n";
    }
}

// Перемещающее присваивание: свои клетки освобождаются, чужие забираются.
Map& Map::operator=(Map&& other) noexcept {
    // Явный this: присваивание самому себе (map = std::move(map)) не должно
    // освободить клетки, которые сейчас же предстоит забрать.
    if (this == &other) {
        return *this;
    }
    if (cells_ != nullptr) {
        --live_count_;
        if (trace) {
            std::cout << "  [Map] destroyed " << name_ << " (replaced)\n";
        }
    }
    delete[] cells_;
    take_from(other);
    if (trace) {
        std::cout << "  [Map] moved " << name_ << " into an existing object\n";
    }
    return *this;
}

void Map::clear() {
    for (int y = 0; y < height_; ++y) {
        for (int x = 0; x < width_; ++x) {
            bool border = x == 0 || y == 0 || x == width_ - 1 || y == height_ - 1;
            cells_[index(x, y)] = border ? WALL : FLOOR;
        }
    }
}

bool Map::inside(int x, int y) const {
    return x >= 0 && y >= 0 && x < width_ && y < height_;
}

char Map::tile(int x, int y) const {
    return inside(x, y) ? cells_[index(x, y)] : WALL;
}

void Map::set_tile(int x, int y, char value) {
    if (inside(x, y)) {
        cells_[index(x, y)] = value;
    }
}

bool Map::walkable(int x, int y) const {
    return tile(x, y) != WALL;   // tile сам проверяет границы
}

void Map::draw(const Hero* hero, const MonsterList* monsters) const {
    for (int y = 0; y < height_; ++y) {
        for (int x = 0; x < width_; ++x) {
            char shown = cells_[index(x, y)];
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
    case LoadStatus::not_found:
        return "file not found";
    case LoadStatus::bad_format:
        return "corrupted format";
    case LoadStatus::bad_size:
        return "invalid size";
    }
    return "unknown error";
}
