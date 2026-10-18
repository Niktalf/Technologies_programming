#pragma once

#include <istream>
#include <string>

#include "color.hpp"

// Результат операции, которая может не получиться.
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

// Практика 10: изображение само управляет своей памятью.
//
// Конструктор получает размеры и выделяет буфер пикселей ровно под них,
// деструктор освобождает его. На практике 9 пиксели лежали в массиве
// наибольшего размера внутри объекта, и объект весил три мегабайта;
// теперь он хранит указатель на буфер в куче и весит несколько байт.
//
// Правило пяти. Класс владеет памятью, поэтому все пять особых методов
// написаны явно. Копирование по умолчанию скопировало бы указатель, и два
// объекта освобождали бы один буфер, — поэтому копия делает собственный
// буфер. Перемещение буфер не копирует, а забирает.
class Image {
public:
    static constexpr int MAX_SIDE = 8192;

    static const char* status_text(ImageStatus status);

    // Пустое изображение 0 на 0 без буфера. Так выглядит редактор, в котором
    // ещё ничего не открыто, и объект, из которого переместили пиксели.
    Image();

    // Чёрное изображение width x height. Неразумный размер —
    // std::invalid_argument с текстом причины; если памяти не хватило,
    // new возбудит std::bad_alloc. В обоих случаях объект не создаётся.
    Image(int width, int height);

    // Копия получает собственный буфер с теми же пикселями.
    Image(const Image& other);
    Image& operator=(const Image& other);

    // Перемещение забирает буфер, источник остаётся пустым изображением.
    // noexcept: перемещение ничего не выделяет и не может бросить исключение.
    Image(Image&& other) noexcept;
    Image& operator=(Image&& other) noexcept;

    ~Image();

    // Подходит ли размер. Конструктор пользуется этой проверкой сам, а снаружи
    // она нужна, чтобы узнать причину отказа заранее.
    static ImageStatus check_size(int width, int height);

    bool empty() const { return pixels_ == nullptr; }
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

    // Преобразования. Результат строится в новом объекте и перемещается
    // на место этого. При ошибке изображение не меняется.
    ImageStatus crop(int x, int y, int width, int height);
    ImageStatus rotate(int angle);
    ImageStatus scale(int width, int height);

    // Чтение и запись PPM. При ошибке чтения изображение не меняется.
    ImageStatus load(const std::string& path, PpmFormat& format);
    ImageStatus save(const std::string& path, PpmFormat format) const;

    // Одна строка: номер объекта, размер, углы, самая тёмная и самая
    // светлая точка. Нужна опытам demo.
    void print_short() const;

    // Сколько изображений владеют буфером прямо сейчас и печатать ли
    // сообщения из конструкторов, деструктора и перемещений.
    static int live_count() { return live_count_; }
    static bool trace;

private:
    // Как разложены пиксели — внутреннее дело класса.
    int offset(int x, int y) const { return y * width_ + x; }
    std::size_t count() const {
        return static_cast<std::size_t>(width_) * static_cast<std::size_t>(height_);
    }

    ImageStatus read_pixels(std::istream& file, PpmFormat format, int maximum);

    // Отдаёт этому объекту буфер и размеры other, оставляя other пустым.
    void take_from(Image& other);
    // Освобождает свой буфер и становится пустым.
    void release();

    int width_ = 0;
    int height_ = 0;
    Color* pixels_ = nullptr;   // width_ * height_ элементов из new[]; nullptr — пусто
    int id_;                    // номер объекта — чтобы по сообщениям видеть, кто есть кто

    static int live_count_;
    static int next_id_;
};
