#include "../../../../../Technologies_programming/Raster_Editor/include/image.hpp"

#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <utility>
#include <vector>

namespace {

// Точка, которую ещё предстоит проверить при заливке.
struct Point {
    int x;
    int y;
};

} // namespace

bool Image::trace_ = false;
int Image::live_count_ = 0;
int Image::next_id_ = 1;

ImageStatus Image::check_size(int width, int height) {
    if (width <= 0 || height <= 0) {
        return ImageStatus::bad_argument;
    }
    if (width > MAX_SIDE || height > MAX_SIDE) {
        return ImageStatus::too_large;
    }
    return ImageStatus::ok;
}

Image::Image()
    : id_(next_id_++) {
}

// Список инициализации: поля получают значения до входа в тело
// конструктора, в порядке объявления в классе. pixels_ пока nullptr.
Image::Image(int width, int height)
    : width_(width), height_(height), id_(next_id_++) {
    // Размеры проверяются до выделения: испорченное число не должно
    // превратиться в запрос на гигабайты.
    ImageStatus status = check_size(width, height);
    if (status != ImageStatus::ok) {
        throw std::invalid_argument(status_text(status));
    }
    // new[] для массива — значит, освобождать нужно delete[], а не delete.
    // При нехватке памяти new возбуждает std::bad_alloc, а не возвращает NULL.
    // Каждый Color при создании чёрный: его поля равны нулю.
    pixels_ = new Color[count()];
    ++live_count_;
    if (trace_) {
        std::cout << "  [Image #" << id_ << "] created " << width_ << " x " << height_ << '\n';
    }
}

Image::Image(const Image& other)
    : width_(other.width_), height_(other.height_), id_(next_id_++) {
    if (!other.empty()) {
        pixels_ = new Color[count()];
        std::copy(other.pixels_, other.pixels_ + count(), pixels_);
        ++live_count_;
    }
    if (trace_) {
        std::cout << "  [Image #" << id_ << "] copy-constructed from #" << other.id_ << '\n';
    }
}

Image& Image::operator=(const Image& other) {
    // Присваивание самому себе: без этой проверки мы освободили бы буфер
    // и потом копировали бы из уже освобождённой памяти.
    if (this == &other) {
        if (trace_) {
            std::cout << "  [Image #" << id_ << "] self-assignment - nothing to do\n";
        }
        return *this;
    }
    // Сначала новый буфер, потом освобождение старого. Если new возбудит
    // исключение, объект останется прежним и целым.
    Color* fresh = nullptr;
    if (!other.empty()) {
        fresh = new Color[other.count()];
        std::copy(other.pixels_, other.pixels_ + other.count(), fresh);
    }
    release();
    pixels_ = fresh;
    width_ = other.width_;
    height_ = other.height_;
    if (pixels_ != nullptr) {
        ++live_count_;
    }
    if (trace_) {
        std::cout << "  [Image #" << id_ << "] copy-assigned from #" << other.id_ << '\n';
    }
    return *this;   // ссылка на себя: можно писать a = b = c
}

void Image::take_from(Image& other) {
    width_ = other.width_;
    height_ = other.height_;
    pixels_ = other.pixels_;
    // Прежний владелец остаётся пустым: его деструктор ничего не освободит.
    other.pixels_ = nullptr;
    other.width_ = 0;
    other.height_ = 0;
}

void Image::release() {
    if (pixels_ != nullptr) {
        --live_count_;
    }
    delete[] pixels_;   // парная форма к new[]; delete[] от nullptr ничего не делает
    pixels_ = nullptr;
    width_ = 0;
    height_ = 0;
}

// Перемещающий конструктор: новый объект забирает буфер. Новой памяти
// не выделяется, пиксели не копируются — переезжает один указатель.
Image::Image(Image&& other) noexcept
    : id_(next_id_++) {
    take_from(other);
    if (trace_) {
        std::cout << "  [Image #" << id_ << "] move-constructed from #" << other.id_ << '\n';
    }
}

Image& Image::operator=(Image&& other) noexcept {
    if (this == &other) {
        return *this;
    }
    release();   // свой буфер больше не нужен
    take_from(other);
    if (trace_) {
        std::cout << "  [Image #" << id_ << "] move-assigned from #" << other.id_ << '\n';
    }
    return *this;
}

Image::~Image() {
    if (trace_) {
        std::cout << "  [Image #" << id_ << "] destroyed" << (empty() ? " (empty)" : "") << '\n';
    }
    release();
}

Color Image::pixel(int x, int y) const {
    if (!inside(x, y)) {
        return Color::from_rgb(0, 0, 0);
    }
    return pixels_[offset(x, y)];
}

bool Image::set_pixel(int x, int y, Color color) {
    if (!inside(x, y)) {
        return false;
    }
    pixels_[offset(x, y)] = color;
    return true;
}

void Image::fill_gradient() {
    // Слева направо от чёрного к белому. Умножение до деления: иначе все
    // значения, кроме последнего, станут нулём.
    for (int y = 0; y < height_; ++y) {
        for (int x = 0; x < width_; ++x) {
            int level = width_ > 1 ? x * 255 / (width_ - 1) : 0;
            set_pixel(x, y, Color::from_rgb(level, level, level));
        }
    }
}

void Image::fill_checker(int cell) {
    if (cell < 1) {
        cell = 1;
    }
    for (int y = 0; y < height_; ++y) {
        for (int x = 0; x < width_; ++x) {
            // Номер клетки по x плюс номер по y: чётность суммы даёт шахматный
            // узор. Если взять только одну из координат, получатся полосы.
            bool dark = ((x / cell) + (y / cell)) % 2 != 0;
            set_pixel(x, y, dark ? Color::from_rgb(40, 40, 40) : Color::from_rgb(220, 220, 220));
        }
    }
}

void Image::fill_stripes(int count) {
    static const std::uint8_t PALETTE[][3] = {
        { 220, 40, 40 }, { 240, 160, 30 }, { 240, 230, 50 }, { 60, 190, 70 },
        { 40, 170, 220 }, { 60, 70, 200 }, { 150, 60, 190 }, { 230, 230, 230 }
    };
    const int palette_size = static_cast<int>(sizeof PALETTE / sizeof PALETTE[0]);
    if (count < 1) {
        count = 1;
    }
    for (int y = 0; y < height_; ++y) {
        // Номер полосы: умножение до деления, как в градиенте.
        int band = y * count / height_;
        const std::uint8_t* c = PALETTE[band % palette_size];
        for (int x = 0; x < width_; ++x) {
            set_pixel(x, y, Color::from_rgb(c[0], c[1], c[2]));
        }
    }
}

void Image::invert() {
    for (int i = 0; i < width_ * height_; ++i) {
        pixels_[i] = pixels_[i].inverted();
    }
}

void Image::brighten(int delta) {
    for (int i = 0; i < width_ * height_; ++i) {
        pixels_[i].brighten(delta);   // метод меняет сам элемент массива, а не копию
    }
}

bool Image::min_max(int& darkest, int& brightest) const {
    if (empty()) {
        return false;
    }
    // Начальные значения — из первого пикселя, а не из нуля: иначе минимум
    // всегда оказался бы нулём.
    darkest = pixels_[0].luminance();
    brightest = darkest;
    for (int i = 1; i < width_ * height_; ++i) {
        int value = pixels_[i].luminance();
        if (value < darkest) {
            darkest = value;
        }
        if (value > brightest) {
            brightest = value;
        }
    }
    return true;
}

FillReport Image::flood_fill(int x, int y, Color color) {
    FillReport report;
    if (!inside(x, y)) {
        return report;
    }
    Color old_color = pixel(x, y);
    // Заливка тем же цветом, что уже есть: перекрашенный пиксель не отличается
    // от неперекрашенного, и заливка никогда бы не закончилась.
    if (old_color.same_as(color)) {
        return report;
    }
    // Стек точек — std::vector: растёт сам, и освобождать его не нужно.
    // Точка перекрашивается в момент, когда кладётся на стек, поэтому одна
    // точка не попадает на стек дважды.
    std::vector<Point> stack;
    set_pixel(x, y, color);
    stack.push_back(Point{ x, y });
    report.filled = 1;
    report.max_stack = 1;
    while (!stack.empty()) {
        Point p = stack.back();
        stack.pop_back();
        const Point neighbours[4] = {
            { p.x + 1, p.y }, { p.x - 1, p.y }, { p.x, p.y + 1 }, { p.x, p.y - 1 }
        };
        for (const Point& next : neighbours) {
            // Сначала границы, потом цвет: за краем цвета нет.
            if (inside(next.x, next.y) && pixel(next.x, next.y).same_as(old_color)) {
                set_pixel(next.x, next.y, color);
                ++report.filled;
                stack.push_back(next);
                if (static_cast<int>(stack.size()) > report.max_stack) {
                    report.max_stack = static_cast<int>(stack.size());
                }
            }
        }
    }
    return report;
}

ImageStatus Image::crop(int x, int y, int width, int height) {
    // Сравнение через вычитание: x + width могло бы переполнить int.
    if (empty() || x < 0 || y < 0 || width <= 0 || height <= 0
        || x > width_ - width || y > height_ - height) {
        return ImageStatus::bad_argument;
    }
    Image result(width, height);
    for (int row = 0; row < height; ++row) {
        for (int col = 0; col < width; ++col) {
            result.set_pixel(col, row, pixel(x + col, y + row));
        }
    }
    // Готовый результат перемещается на место этого изображения: старый
    // буфер освобождается, новый переходит к *this без копирования.
    *this = std::move(result);
    return ImageStatus::ok;
}

ImageStatus Image::rotate(int angle) {
    if (empty() || angle % 90 != 0) {
        return ImageStatus::bad_argument;
    }
    // Число четвертей оборота от 0 до 3: -90 — это то же, что 270.
    int turns = ((angle / 90) % 4 + 4) % 4;
    if (turns == 0) {
        return ImageStatus::ok;
    }
    Image result(turns == 2 ? width_ : height_, turns == 2 ? height_ : width_);
    for (int y = 0; y < height_; ++y) {
        for (int x = 0; x < width_; ++x) {
            Color color = pixel(x, y);
            if (turns == 1) {
                result.set_pixel(height_ - 1 - y, x, color);   // 90 по часовой
            } else if (turns == 2) {
                result.set_pixel(width_ - 1 - x, height_ - 1 - y, color);
            } else {
                result.set_pixel(y, width_ - 1 - x, color);    // 90 против часовой
            }
        }
    }
    *this = std::move(result);
    return ImageStatus::ok;
}

ImageStatus Image::scale(int width, int height) {
    if (empty()) {
        return ImageStatus::bad_argument;
    }
    ImageStatus status = check_size(width, height);
    if (status != ImageStatus::ok) {
        return status;
    }
    Image result(width, height);
    // Ближайший сосед: для каждой точки нового изображения берём ближайшую
    // точку старого. Умножение до деления.
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            result.set_pixel(x, y, pixel(x * width_ / width, y * height_ / height));
        }
    }
    *this = std::move(result);
    return ImageStatus::ok;
}

void Image::print_short() const {
    std::cout << "  #" << id_ << ": " << width_ << " x " << height_;
    if (empty()) {
        std::cout << ", empty\n";
        return;
    }
    int darkest = 0;
    int brightest = 0;
    min_max(darkest, brightest);
    Color first = pixel(0, 0);
    Color last = pixel(width_ - 1, height_ - 1);
    std::cout << ", top left " << static_cast<int>(first.red()) << ' '
              << static_cast<int>(first.green()) << ' ' << static_cast<int>(first.blue())
              << ", bottom right " << static_cast<int>(last.red()) << ' '
              << static_cast<int>(last.green()) << ' ' << static_cast<int>(last.blue())
              << ", brightness " << darkest << "-" << brightest << '\n';
}
