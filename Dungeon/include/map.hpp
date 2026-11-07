#pragma once

#include <istream>
#include <string>

class Hero;
class MonsterList;

// Результат загрузки уровня: те же причины, что у Status в версии на C.
enum class LoadStatus { ok, not_found, bad_format, bad_size };

// Практика 9: карта — класс. Поля: размеры и содержимое. Методы: получить
// клетку, задать клетку, проверить проходимость, вывести на экран.
//
// Чем класс отличается от структуры Map из версии на C. Там данные лежали
// в структуре, а функции map_at, map_set жили отдельно и получали адрес
// карты первым параметром. Здесь функции стали методами: они объявлены
// внутри класса и вызываются через объект — map.tile(x, y). Поля закрыты
// (private): изменить клетку в обход set_tile снаружи нельзя.
//
// Размер карты задаётся при запуске, но память под клетки пока не выделяется:
// поле cells_ — массив наибольшего размера 60 на 120, и карта занимает его
// левый верхний угол width_ на height_. Выделять ровно столько, сколько
// нужно, класс научится на практике 10, вместе с конструктором и деструктором.
class Map {
public:
    static constexpr int MIN_SIDE = 5;
    static constexpr int MAX_WIDTH = 120;
    static constexpr int MAX_HEIGHT = 60;

    static constexpr char WALL = '#';
    static constexpr char FLOOR = '.';
    static constexpr char GOLD = '$';
    static constexpr char EXIT = '>';

    // Пустая комната width x height: стены по периметру, внутри пол.
    // Возвращает false и не меняет карту, если размер недопустим.
    // Конструкторов пока нет — начальное состояние задаёт этот метод.
    bool reset(int width, int height);

    // Загружает уровень из текстового файла в формате практики 8:
    // # . $ > — клетки, @ — герой (ровно один), M — монстр. Монстры
    // уходят в список monsters, на карте на их месте остаётся пол.
    // При ошибке не меняются ни карта, ни список, а в bad_line записывается
    // номер строки файла с ошибкой (0, если ошибка не относится к одной строке).
    LoadStatus load(const std::string& path, int& bad_line, MonsterList& monsters);

    int width() const { return width_; }
    int height() const { return height_; }

    // Клетка, где стоял герой в файле уровня; у случайного уровня — (1, 1).
    int start_x() const { return start_x_; }
    int start_y() const { return start_y_; }

    // Клетка; за пределами карты — стена.
    char tile(int x, int y) const;
    void set_tile(int x, int y, char value);
    bool walkable(int x, int y) const;
    bool inside(int x, int y) const;

    // Рисует карту, поверх неё монстров и героя. О герое и монстрах карте
    // нужно знать только одно — где они стоят, — и больше она ничего у них
    // не спрашивает. Сама карта при этом не меняется.
    void draw(const Hero* hero, const MonsterList* monsters) const;

    // Текст для результата загрузки — те же слова, что в версии на C.
    static const char* status_text(LoadStatus status);

private:
    // Шаги загрузки. Оба работают с временной картой внутри load.
    LoadStatus read_rows(std::istream& file, int& bad_line);
    LoadStatus find_hero(int& bad_line, MonsterList& monsters);

    static bool is_tile(char c);

    char cells_[MAX_HEIGHT][MAX_WIDTH] = {};   // cells_[y][x]: строка, затем столбец
    int width_ = 0;
    int height_ = 0;
    int start_x_ = 1;
    int start_y_ = 1;
};
