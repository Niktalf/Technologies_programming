#include "ppm_file.hpp"

#include <cctype>
#include <fstream>
#include <utility>

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

} // namespace

// Читает и проверяет заголовок: сигнатуру, размеры и максимум канала.
ImageStatus PpmFile::read_header(std::istream& file, PpmFormat& format, int& width,
                                 int& height, int& maximum) {
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

ImageStatus PpmFile::read_pixels(std::istream& file, PpmFormat format, int maximum,
                                 Image& image) {
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            int c[3];
            // Файл оборван или значение больше объявленного максимума.
            if (!read_channels(file, format, c)
                || c[0] > maximum || c[1] > maximum || c[2] > maximum) {
                return ImageStatus::corrupt;
            }
            image.set_pixel(x, y, Color::from_rgb(scale_channel(c[0], maximum),
                                                  scale_channel(c[1], maximum),
                                                  scale_channel(c[2], maximum)));
        }
    }
    return ImageStatus::ok;
}

ImageStatus PpmFile::read(const std::string& path, Image& image, PpmFormat& format) {
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
    // временное разрушится само при выходе из метода, а image не изменится.
    Image loaded(width, height);
    status = read_pixels(file, read_format, maximum, loaded);
    if (status != ImageStatus::ok) {
        return status;
    }
    image = std::move(loaded);
    format = read_format;
    return ImageStatus::ok;
}   // file закрывается здесь сам: деструктор std::ifstream

ImageStatus PpmFile::write(const std::string& path, const Image& image, PpmFormat format) {
    if (image.empty()) {
        return ImageStatus::bad_argument;
    }
    std::ofstream file(path, std::ios::binary);
    if (!file) {
        return ImageStatus::cannot_open;
    }
    // Заголовок: сигнатура, ширина и высота, максимальное значение канала.
    file << 'P' << (format == PpmFormat::binary ? '6' : '3') << '\n'
         << image.width() << ' ' << image.height() << '\n' << Color::CHANNEL_MAX << '\n';
    // В тексте числа разделяются пробелами, строки изображения — переводом
    // строки. Без разделителей файл не откроет ни один просмотрщик.
    for (int y = 0; y < image.height() && file; ++y) {
        for (int x = 0; x < image.width() && file; ++x) {
            Color c = image.pixel(x, y);
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
