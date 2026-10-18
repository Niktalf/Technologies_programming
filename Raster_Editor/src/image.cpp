#include "image.hpp"

#include <cctype>
#include <fstream>
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
        return "too large: at most 1024 x 1024 in this version";
    case ImageStatus::bad_argument:
        return "invalid argument";
    case ImageStatus::write_failed:
        return "write failed";
    }
    return "unknown error";
}

ImageStatus Image::reset(int width, int height) {
    if (width <= 0 || height <= 0) {
        return ImageStatus::bad_argument;
    }
    if (width > MAX_SIDE || height > MAX_SIDE) {
        return ImageStatus::too_large;
    }
    width_ = width;
    height_ = height;
    for (int i = 0; i < width_ * height_; ++i) {
        pixels_[i] = Color::from_rgb(0, 0, 0);
    }
    return ImageStatus::ok;
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

ImageStatus Image::crop_from(const Image& source, int x, int y, int width, int height) {
    // Явный this: источник и результат должны быть разными объектами,
    // иначе мы перезаписали бы пиксели, которые ещё не прочитаны.
    if (this == &source) {
        return ImageStatus::bad_argument;
    }
    // Сравнение через вычитание: x + width могло бы переполнить int.
    if (x < 0 || y < 0 || width <= 0 || height <= 0
        || x > source.width_ - width || y > source.height_ - height) {
        return ImageStatus::bad_argument;
    }
    width_ = width;
    height_ = height;
    for (int row = 0; row < height; ++row) {
        for (int col = 0; col < width; ++col) {
            pixels_[offset(col, row)] = source.pixel(x + col, y + row);
        }
    }
    return ImageStatus::ok;
}

ImageStatus Image::rotate_from(const Image& source, int angle) {
    if (this == &source || angle % 90 != 0) {
        return ImageStatus::bad_argument;
    }
    // Число четвертей оборота от 0 до 3: -90 — это то же, что 270.
    int turns = ((angle / 90) % 4 + 4) % 4;
    width_ = turns % 2 == 0 ? source.width_ : source.height_;
    height_ = turns % 2 == 0 ? source.height_ : source.width_;
    for (int y = 0; y < source.height_; ++y) {
        for (int x = 0; x < source.width_; ++x) {
            Color color = source.pixel(x, y);
            if (turns == 0) {
                set_pixel(x, y, color);
            } else if (turns == 1) {
                set_pixel(source.height_ - 1 - y, x, color);   // 90 по часовой
            } else if (turns == 2) {
                set_pixel(source.width_ - 1 - x, source.height_ - 1 - y, color);
            } else {
                set_pixel(y, source.width_ - 1 - x, color);    // 90 против часовой
            }
        }
    }
    return ImageStatus::ok;
}

ImageStatus Image::scale_from(const Image& source, int width, int height) {
    if (this == &source || source.empty() || width <= 0 || height <= 0) {
        return ImageStatus::bad_argument;
    }
    if (width > MAX_SIDE || height > MAX_SIDE) {
        return ImageStatus::too_large;
    }
    width_ = width;
    height_ = height;
    // Ближайший сосед: для каждой точки нового изображения берём ближайшую
    // точку старого. Умножение до деления.
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            pixels_[offset(x, y)] =
                source.pixel(x * source.width_ / width, y * source.height_ / height);
        }
    }
    return ImageStatus::ok;
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

ImageStatus Image::load(const std::string& path, PpmFormat& format) {
    // Двоичный режим всегда: для P6 он обязателен, потому что в текстовом
    // некоторые системы портят байты, совпадающие с кодом перевода строки.
    // Для P3 он безвреден.
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return ImageStatus::cannot_open;
    }
    int width = 0;
    int height = 0;
    int maximum = 0;
    ImageStatus status = read_header(file, format, width, height, maximum);
    if (status == ImageStatus::ok) {
        status = reset(width, height);
    }
    if (status == ImageStatus::ok) {
        status = read_pixels(file, format, maximum);
    }
    // Файл закрывается сам, когда переменная file выходит из области
    // видимости. Как это устроено, разберём на практике 10.
    return status;
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
