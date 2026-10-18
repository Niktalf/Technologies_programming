#include "image.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <utility>
#include <vector>

namespace {

// Больше этого числа в заголовке и данных PPM не бывает: максимум
// канала — 65535, а стороны мы всё равно проверим отдельно.
constexpr long PPM_NUMBER_MAX = 1000000;

// Пропускает пробелы и строки комментариев. Символ, который оказался
// не пробелом и не решёткой, возвращается обратно в поток: читать его
// будет уже тот, кому он нужен.
void skip_spaces_and_comments(std::istream& file) {
    int c = file.get();
    while (c == '#' || (c != EOF && std::isspace(c))) {
        if (c == '#') {
            while (c != EOF && c != '\n') {   // комментарий — до конца строки
                c = file.get();
            }
        }
        c = file.get();
    }
    if (c != EOF) {
        file.unget();
    }
}

// Читает одно неотрицательное целое, собирая цифры самостоятельно:
// file >> int на огромном числе просто откажет, не объяснив почему, а файл
// с шириной в сто миллиардов должен давать понятную ошибку. Возвращает
// false, если цифр нет или число больше PPM_NUMBER_MAX.
bool read_number(std::istream& file, int& out) {
    skip_spaces_and_comments(file);
    long value = 0;
    int digits = 0;
    bool too_big = false;
    int c = file.get();
    while (c != EOF && std::isdigit(c) && !too_big) {
        value = value * 10 + (c - '0');
        too_big = value > PPM_NUMBER_MAX;   // проверка на каждой цифре
        ++digits;
        c = file.get();
    }
    // Символ после числа не наш: возвращаем его в поток. В P6 это тот самый
    // единственный пробел перед байтами данных.
    if (c != EOF) {
        file.unget();
    }
    if (digits == 0 || too_big) {
        return false;
    }
    out = static_cast<int>(value);
    return true;
}

// Читает три канала одного пикселя текстом или байтами.
// Возвращает false, если файл оборвался.
bool read_channels(std::istream& file, PpmFormat format, int channels[3]) {
    if (format == PpmFormat::text) {
        return read_number(file, channels[0]) && read_number(file, channels[1])
               && read_number(file, channels[2]);
    }
    char bytes[3];
    if (!file.read(bytes, 3)) {
        return false;
    }
    for (int i = 0; i < 3; ++i) {
        channels[i] = static_cast<unsigned char>(bytes[i]);
    }
    return true;
}

// Приводит канал к диапазону 0..255. Умножение до деления: иначе всё
// станет нулём.
std::uint8_t scale_channel(int value, int maximum) {
    if (maximum != Color::CHANNEL_MAX) {
        value = value * Color::CHANNEL_MAX / maximum;
    }
    return Color::clamp(value);
}

// Читает и проверяет заголовок: сигнатуру, размеры и максимум канала.
ImageStatus read_header(std::istream& file, PpmFormat& format, int& width, int& height,
                        int& maximum) {
    skip_spaces_and_comments(file);
    char signature[2] = { 0, 0 };
    if (!file.get(signature[0]) || !file.get(signature[1]) || signature[0] != 'P'
        || (signature[1] != '3' && signature[1] != '6')) {
        return ImageStatus::bad_format;
    }
    format = signature[1] == '6' ? PpmFormat::binary : PpmFormat::text;
    if (!read_number(file, width) || !read_number(file, height) || !read_number(file, maximum)) {
        return ImageStatus::bad_format;
    }
    if (width <= 0 || height <= 0 || maximum <= 0 || maximum > 65535) {
        return ImageStatus::bad_format;
    }
    // В двоичной разновидности при максимуме больше 255 канал занимает
    // два байта. Такие файлы редки, и мы их честно не поддерживаем.
    if (format == PpmFormat::binary && maximum > Color::CHANNEL_MAX) {
        return ImageStatus::bad_format;
    }
    if (width > Image::MAX_SIDE || height > Image::MAX_SIDE) {
        return ImageStatus::too_large;
    }
    // В двоичном файле после максимума идёт ровно один пробельный символ,
    // а дальше сразу байты. Пропускать пробелы дальше нельзя: первый байт
    // данных может совпасть с кодом пробела.
    if (format == PpmFormat::binary && file.get() == EOF) {
        return ImageStatus::corrupt;
    }
    return ImageStatus::ok;
}

// Точка, которую ещё предстоит проверить при заливке.
struct Point {
    int x;
    int y;
};

} // namespace

bool Image::trace = false;
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
    if (trace) {
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
    if (trace) {
        std::cout << "  [Image #" << id_ << "] copy-constructed from #" << other.id_ << '\n';
    }
}

Image& Image::operator=(const Image& other) {
    // Присваивание самому себе: без этой проверки мы освободили бы буфер
    // и потом копировали бы из уже освобождённой памяти.
    if (this == &other) {
        if (trace) {
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
    if (trace) {
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
    if (trace) {
        std::cout << "  [Image #" << id_ << "] move-constructed from #" << other.id_ << '\n';
    }
}

Image& Image::operator=(Image&& other) noexcept {
    if (this == &other) {
        return *this;
    }
    release();   // свой буфер больше не нужен
    take_from(other);
    if (trace) {
        std::cout << "  [Image #" << id_ << "] move-assigned from #" << other.id_ << '\n';
    }
    return *this;
}

Image::~Image() {
    if (trace) {
        std::cout << "  [Image #" << id_ << "] destroyed" << (empty() ? " (empty)" : "") << '\n';
    }
    release();
}

const char* Image::status_text(ImageStatus status) {
    switch (status) {
    case ImageStatus::ok:
        return "success";
    case ImageStatus::cannot_open:
        return "cannot open the file";
    case ImageStatus::bad_format:
        return "invalid format: not a PPM or the header is corrupted";
    case ImageStatus::corrupt:
        return "corrupted data: the file is cut off or a value is out of range";
    case ImageStatus::too_large:
        return "too large: at most 8192 x 8192";
    case ImageStatus::bad_argument:
        return "invalid argument";
    case ImageStatus::write_failed:
        return "write failed";
    }
    return "unknown error";
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
            auto level = static_cast<std::uint8_t>(width_ > 1 ? x * 255 / (width_ - 1) : 0);
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

ImageStatus Image::read_pixels(std::istream& file, PpmFormat format, int maximum) {
    for (int y = 0; y < height_; ++y) {
        for (int x = 0; x < width_; ++x) {
            int c[3];
            // Файл оборван или значение больше объявленного максимума.
            if (!read_channels(file, format, c)
                || c[0] > maximum || c[1] > maximum || c[2] > maximum) {
                return ImageStatus::corrupt;
            }
            set_pixel(x, y, Color::from_rgb(scale_channel(c[0], maximum),
                                            scale_channel(c[1], maximum),
                                            scale_channel(c[2], maximum)));
        }
    }
    return ImageStatus::ok;
}

ImageStatus Image::save(const std::string& path, PpmFormat format) const {
    if (empty()) {
        return ImageStatus::bad_argument;
    }
    std::ofstream file(path, std::ios::binary);
    if (!file) {
        return ImageStatus::cannot_open;
    }
    // Заголовок: сигнатура, ширина и высота, максимальное значение канала.
    file << 'P' << (format == PpmFormat::binary ? '6' : '3') << '\n'
         << width_ << ' ' << height_ << '\n' << Color::CHANNEL_MAX << '\n';
    // В тексте числа разделяются пробелами, строки изображения — переводом
    // строки. Без разделителей файл не откроет ни один просмотрщик.
    for (int y = 0; y < height_ && file; ++y) {
        for (int x = 0; x < width_ && file; ++x) {
            Color c = pixel(x, y);
            if (format == PpmFormat::text) {
                file << static_cast<int>(c.red()) << ' ' << static_cast<int>(c.green()) << ' '
                     << static_cast<int>(c.blue()) << ' ';
            } else {
                file.put(static_cast<char>(c.red()));
                file.put(static_cast<char>(c.green()));
                file.put(static_cast<char>(c.blue()));
            }
        }
        if (format == PpmFormat::text) {
            file << '\n';
        }
    }
    // Часть ошибок записи видна только при закрытии: буфер уходит на диск
    // именно тогда. Поэтому файл закрывается явно и проверяется после.
    file.close();
    return file ? ImageStatus::ok : ImageStatus::write_failed;
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

ImageStatus Image::load(const std::string& path, PpmFormat& format) {
    // Двоичный режим всегда: для P6 он обязателен, потому что в текстовом
    // некоторые системы портят байты, совпадающие с кодом перевода строки.
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return ImageStatus::cannot_open;
    }
    int width = 0;
    int height = 0;
    int maximum = 0;
    PpmFormat read_format = PpmFormat::text;
    ImageStatus status = read_header(file, read_format, width, height, maximum);
    if (status != ImageStatus::ok) {
        return status;
    }
    // Пиксели читаются во временное изображение. Если файл оборван,
    // временное разрушится само при выходе из метода, а это — не изменится.
    Image loaded(width, height);
    status = loaded.read_pixels(file, read_format, maximum);
    if (status != ImageStatus::ok) {
        return status;
    }
    *this = std::move(loaded);
    format = read_format;
    return ImageStatus::ok;
}   // file закрывается здесь сам: деструктор std::ifstream

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
