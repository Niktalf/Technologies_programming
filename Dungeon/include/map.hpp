#pragma once

#include <istream>
#include <stdexcept>
#include <string>
#include <vector>

class Hero;
class MonsterList;

// Причина, по которой уровень из файла не загрузился: те же, что у Status
// в версии на C и у LoadStatus на практике 9.
enum class LoadStatus { not_found, bad_format, bad_size };

// Ошибка в файле уровня. Наследник std::runtime_error: ловится и как
// LevelError, и как любое std::exception. Кроме текста хранит причину
// и номер строки файла (0, если ошибка не относится к одной строке).
class LevelError : public std::runtime_error {
public:
    LevelError(LoadStatus status, int line);

    LoadStatus status() const { return status_; }
    int line() const { return line_; }

private:
    // Текст для what(): «corrupted format (line 3)».
    static std::string describe(LoadStatus status, int line);

    LoadStatus status_;
    int line_;
};

// Практика 10: карта сама выделяет и освобождает память под клетки.
//
// Конструктор получает размеры (или файл уровня) и выделяет ровно
// width * height символов через new[]. Деструктор освобождает их через
// delete[]. На практике 9 клетки лежали в массиве наибольшего размера
// внутри объекта; теперь объект хранит только указатель на блок в куче.
//
// Правило пяти. Класс владеет памятью, поэтому все пять особых методов
// объявлены явно: деструктор, копирующие конструктор и присваивание,
// перемещающие конструктор и присваивание. Копировать карту запрещено,
// перемещать — можно: перемещение передаёт блок клеток другому объекту,
// а прежний владелец остаётся пустым.
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
    // Неразумный размер — std::invalid_argument, и объект не создаётся.
    Map(int width, int height);

    // Уровень из текстового файла в формате практики 8: # . $ > — клетки,
    // @ — герой (ровно один), M — монстр. Монстры уходят в список monsters,
    // на карте на их месте остаётся пол. При любой ошибке в файле —
    // LevelError: объект карты не создаётся, список монстров не меняется.
    Map(const std::string& path, MonsterList& monsters);

    ~Map();

    // Копирование запрещено. Копия по умолчанию скопировала бы указатель
    // cells_, и два деструктора освободили бы один блок. Настоящая копия
    // карте не нужна: в игре карта одна.
    Map(const Map&) = delete;
    Map& operator=(const Map&) = delete;

    // Перемещение разрешено: блок клеток переходит к новому владельцу,
    // а у прежнего остаётся пустая карта 0 на 0, которую можно только
    // уничтожить или переприсвоить. noexcept: перемещение ничего не выделяет
    // и потому не может бросить исключение.
    Map(Map&& other) noexcept;
    Map& operator=(Map&& other) noexcept;

    int width() const { return width_; }
    int height() const { return height_; }

    // Клетка, где стоял герой в файле уровня; у пустой комнаты — (1, 1).
    int start_x() const { return start_x_; }
    int start_y() const { return start_y_; }

    // Снова делает карту пустой комнатой того же размера. Нужен генератору:
    // каждую попытку он начинает с чистой комнаты.
    void clear();

    // Клетка; за пределами карты — стена.
    char tile(int x, int y) const;
    void set_tile(int x, int y, char value);
    bool walkable(int x, int y) const;
    bool inside(int x, int y) const;

    // Рисует карту, поверх неё монстров и героя.
    void draw(const Hero* hero, const MonsterList* monsters) const;

    // Сколько карт владеют клетками прямо сейчас и печатать ли сообщения
    // из конструкторов, деструктора и перемещений. Статические члены
    // принадлежат классу, а не объекту: они одни на все карты.
    static int live_count() { return live_count_; }
    static bool trace;

    static const char* status_text(LoadStatus status);

private:
    // Читает строки файла и проверяет форму: размеры, одинаковую длину строк.
    static std::vector<std::string> read_rows(std::istream& file);
    static bool is_tile(char c);

    int index(int x, int y) const { return y * width_ + x; }

    // Забирает клетки и размеры у other, оставляя other пустым.
    void take_from(Map& other);

    int width_ = 0;
    int height_ = 0;
    int start_x_ = 1;
    int start_y_ = 1;
    char* cells_ = nullptr;   // width_ * height_ символов из new[]; nullptr — пустая карта
    std::string name_;        // для отладочных сообщений

    static int live_count_;
};
