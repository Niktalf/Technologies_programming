#pragma once

#include <istream>
#include <string>

#include "color.hpp"

// Результат операции, которая может не получиться. Те же причины, что
// у ImgStatus в версии на C, только нехватки памяти больше нет: память
// здесь не выделяется. Вместо неё — слишком большое изображение.
enum class ImageStatus { ok, cannot_open, bad_format, corrupt, too_large, bad_argument,
                         write_failed };

// Разновидность файла PPM: текстовая P3 или двоичная P6.
enum class PpmFormat { text, binary };

// Итог заливки: сколько точек перекрашено и сколько их ждало проверки
// одновременно.
struct FillReport {
    int filled = 0;
    int max_stack = 0;
};

// Практика 9: изображение — класс.
//
// Размер задаётся командами, но память под пиксели пока не выделяется:
// поле pixels_ — массив наибольшего размера, 1024 на 1024 точки, и
// изображение занимает его начало, width_ * height_ точек подряд. Объект
// весит три мегабайта, поэтому в программе изображения не локальные
// переменные, а поля редактора, который создаётся один раз как static.
// Выделять ровно столько, сколько нужно, класс научится на практике 10.
class Image {
public:
    static constexpr int MAX_SIDE = 1024;

    static const char* status_text(ImageStatus status);

    // Чёрное изображение width x height. Неверный размер — bad_argument
    // или too_large, и изображение не меняется. Конструкторов пока нет —
    // начальное состояние задаёт этот метод.
    ImageStatus reset(int width, int height);

    bool empty() const { return width_ == 0; }
    int width() const { return width_; }
    int height() const { return height_; }
    bool inside(int x, int y) const { return x >= 0 && y >= 0 && x < width_ && y < height_; }

    // Пиксель по координатам. За пределами изображения — чёрный.
    Color pixel(int x, int y) const;

    // Ставит пиксель. Возвращает false, если точка вне изображения.
    bool set_pixel(int x, int y, Color color);

    // Рисунки поверх того размера, какой есть.
    void fill_gradient();
    void fill_checker(int cell);
    void fill_stripes(int count);

    void invert();
    void brighten(int delta);

    // Самая тёмная и самая светлая точка. false у пустого изображения.
    bool min_max(int& darkest, int& brightest) const;

    // Заливка области одного цвета, начиная с точки (x, y). Без рекурсии:
    // точки, которые ещё нужно проверить, лежат в собственном стеке.
    FillReport flood_fill(int x, int y, Color color);

    // Преобразования «из другого изображения»: этот объект становится
    // обрезанной, повёрнутой или масштабированной копией source. Источник
    // и результат должны быть разными объектами: на месте повернуть
    // изображение 300 на 200 нельзя — меняется форма. При ошибке этот
    // объект не меняется.
    ImageStatus crop_from(const Image& source, int x, int y, int width, int height);
    ImageStatus rotate_from(const Image& source, int angle);
    ImageStatus scale_from(const Image& source, int width, int height);

    // Чтение и запись PPM. При ошибке чтения содержимое объекта
    // не определено, поэтому редактор читает во вспомогательное изображение
    // и переносит его в рабочее только при успехе.
    ImageStatus load(const std::string& path, PpmFormat& format);
    ImageStatus save(const std::string& path, PpmFormat format) const;

private:
    // Как разложены пиксели — внутреннее дело класса. Снаружи о смещении
    // никто не знает, поэтому метод закрыт.
    int offset(int x, int y) const { return y * width_ + x; }

    ImageStatus read_pixels(std::istream& file, PpmFormat format, int maximum);

    Color pixels_[MAX_SIDE * MAX_SIDE];   // при создании все чёрные: поля Color равны нулю
    int width_ = 0;
    int height_ = 0;
};
