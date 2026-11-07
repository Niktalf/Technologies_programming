#include "editor.hpp"

#include <cctype>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace {

// Сравнивает название команды без учёта регистра. Аргументы командной
// строки менять нельзя, поэтому регистр учитывается здесь, а не при чтении.
bool is_command(const std::string& word, const char* name) {
    std::string lower = word;
    for (char& c : lower) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return lower == name;
}

// Целое число из слова: всё слово — число, и оно помещается в int.
// Знак «+» и «-» понимается. Строковый поток сам сообщит о переполнении.
bool parse_int(const std::string& word, int& out) {
    std::istringstream in(word);
    int value = 0;
    if (!(in >> value) || !in.eof()) {
        return false;
    }
    out = value;
    return true;
}

// Несколько чисел подряд из words, начиная со слова first. При ошибке
// печатает, какое слово не число.
bool parse_ints(const std::vector<std::string>& words, std::size_t first, int* out, int count) {
    for (int i = 0; i < count; ++i) {
        const std::string& word = words[first + static_cast<std::size_t>(i)];
        if (!parse_int(word, out[i])) {
            std::cout << "Not a number: " << word << '\n';
            return false;
        }
    }
    return true;
}

// Цвет как 0xRRGGBB: шесть шестнадцатеричных цифр с ведущими нулями.
// Флаги потока после вывода возвращаются: иначе все следующие числа
// тоже печатались бы шестнадцатеричными.
void print_hex(Color color) {
    std::ios::fmtflags flags = std::cout.flags();
    char fill = std::cout.fill();
    std::cout << "0x" << std::hex << std::uppercase << std::setw(6) << std::setfill('0')
              << color.packed();
    std::cout.flags(flags);
    std::cout.fill(fill);
}

void print_fill_report(FillReport report) {
    std::cout << "Pixels filled: " << report.filled
              << ", largest stack of pending points: " << report.max_stack << '\n';
    if (report.filled == 0) {
        std::cout << "Nothing changed: the point already has this color.\n";
    }
}

} // namespace

void Editor::print_help() {
    std::cout << "Available operations:\n"
              << "  info                     - information about the image\n"
              << "  gen gradient             - gradient from black to white\n"
              << "  gen checker [cell]       - checkerboard, cell 32 by default\n"
              << "  gen stripes [count]      - color stripes, 8 by default\n"
              << "  invert                   - invert\n"
              << "  bright <number>          - brightness, e.g. bright 40 or bright -40\n"
              << "  size <w> <h>             - create an empty image\n"
              << "  crop <x> <y> <w> <h>     - crop\n"
              << "  scale <w> <h>            - resize\n"
              << "  rotate <angle>           - rotate by a multiple of 90 degrees\n"
              << "  fill <x> <y> <r> <g> <b> - fill an area with a color\n"
              << "  filltest                 - fill on checker, gradient and plain images\n"
              << "  minmax                   - the darkest and the brightest point\n"
              << "  load <file>              - read a PPM, either P3 or P6\n"
              << "  save <file> [text|bin]   - write, text P3 by default\n"
              << "  colors                   - check the color operations\n"
              << "  help                     - this list\n"
              << "  quit                     - exit\n";
}

bool Editor::require_image() const {
    if (!loaded_) {
        std::cout << "No image. Create one with gen.\n";
        return false;
    }
    return true;
}

bool Editor::ensure_image(int width, int height) {
    if (!image_.empty() && image_.width() == width && image_.height() == height) {
        return true;
    }
    // reset сам проверяет размер и при ошибке изображение не трогает.
    ImageStatus status = image_.reset(width, height);
    if (status != ImageStatus::ok) {
        std::cout << "Cannot create an image " << width << " x " << height << ": "
                  << Image::status_text(status) << '\n';
        return false;
    }
    return true;
}

void Editor::take_result() {
    // Присваивание копирует объект целиком — все три мегабайта массива,
    // даже если картинка маленькая: массив — часть объекта. На практике 10
    // пиксели переедут в кучу, и копировать придётся только нужное.
    image_ = scratch_;
}

void Editor::print_info() const {
    std::cout << "File: " << (path_.empty() ? "not set" : path_) << '\n';
    if (!loaded_) {
        std::cout << "No image. Create one with gen.\n";
        return;
    }
    std::cout << "Size: " << image_.width() << " x " << image_.height() << '\n';
    std::cout << "Top left pixel: ";
    print_hex(image_.pixel(0, 0));
    std::cout << "\nBottom right pixel: ";
    print_hex(image_.pixel(image_.width() - 1, image_.height() - 1));
    std::cout << '\n';
}

void Editor::command_size(const std::vector<std::string>& words) {
    int size[2] = { 0, 0 };
    if (words.size() != 3 || !parse_int(words[1], size[0]) || !parse_int(words[2], size[1])) {
        std::cout << "Usage: size <width> <height>\n";
        return;
    }
    if (!ensure_image(size[0], size[1])) {
        return;
    }
    loaded_ = true;
    std::cout << "Image created " << size[0] << " x " << size[1] << " ("
              << static_cast<long>(size[0]) * size[1] * static_cast<long>(sizeof(Color))
              << " bytes)\n";
}

void Editor::command_gen(const std::vector<std::string>& words) {
    if (words.size() < 2) {
        std::cout << "Specify the kind: gradient, checker or stripes\n";
        return;
    }
    // Необязательный числовой параметр: размер клетки или число полос.
    bool has_parameter = words.size() >= 3;
    int parameter = 0;
    if (has_parameter && !parse_int(words[2], parameter)) {
        std::cout << "Not a number: " << words[2] << '\n';
        return;
    }
    // Генератор рисует на изображении того размера, какой есть: размер
    // задаёт команда size, load или crop. Если изображения ещё нет,
    // создаётся 256 на 256.
    if (image_.empty() && !ensure_image(256, 256)) {
        return;
    }
    const std::string& kind = words[1];
    if (is_command(kind, "gradient")) {
        image_.fill_gradient();
    } else if (is_command(kind, "checker")) {
        image_.fill_checker(has_parameter ? parameter : 32);
    } else if (is_command(kind, "stripes")) {
        image_.fill_stripes(has_parameter ? parameter : 8);
    } else {
        std::cout << "Unknown kind: " << kind << '\n';
        return;
    }
    loaded_ = true;
    std::cout << "Image created " << image_.width() << " x " << image_.height() << ".\n";
}

void Editor::command_invert() {
    if (require_image()) {
        image_.invert();
        std::cout << "Inverted.\n";
    }
}

void Editor::command_bright(const std::vector<std::string>& words) {
    if (!require_image()) {
        return;
    }
    if (words.size() < 2) {
        std::cout << "Specify a number, e.g. bright 40\n";
        return;
    }
    if (words.size() > 2) {
        std::cout << "Too many arguments: bright takes one number\n";
        return;
    }
    int delta = 0;
    if (!parse_int(words[1], delta)) {
        std::cout << "Not a number: " << words[1] << ". Example: bright 40 or bright -40\n";
        return;
    }
    image_.brighten(delta);
    std::cout << "Brightness changed by " << delta << ".\n";
}

void Editor::command_crop(const std::vector<std::string>& words) {
    if (!require_image()) {
        return;
    }
    if (words.size() != 5) {
        std::cout << "Usage: crop <x> <y> <width> <height>\n";
        return;
    }
    int v[4] = { 0, 0, 0, 0 };
    if (!parse_ints(words, 1, v, 4)) {
        return;
    }
    if (scratch_.crop_from(image_, v[0], v[1], v[2], v[3]) != ImageStatus::ok) {
        std::cout << "Crop failed: the rectangle must lie inside the image.\n";
        return;
    }
    take_result();
    std::cout << "Cropped to " << image_.width() << " x " << image_.height() << '\n';
}

void Editor::command_rotate(const std::vector<std::string>& words) {
    if (!require_image()) {
        return;
    }
    if (words.size() != 2) {
        std::cout << "Usage: rotate <angle>, a multiple of 90: rotate 90, rotate -90\n";
        return;
    }
    int angle = 0;
    if (!parse_int(words[1], angle)) {
        std::cout << "Not a number: " << words[1] << '\n';
        return;
    }
    if (scratch_.rotate_from(image_, angle) != ImageStatus::ok) {
        std::cout << "The angle must be a multiple of 90 degrees.\n";
        return;
    }
    take_result();
    std::cout << "Rotated by " << angle << ". Size: " << image_.width() << " x "
              << image_.height() << '\n';
}

void Editor::command_scale(const std::vector<std::string>& words) {
    if (!require_image()) {
        return;
    }
    int size[2] = { 0, 0 };
    if (words.size() != 3 || !parse_int(words[1], size[0]) || !parse_int(words[2], size[1])) {
        std::cout << "Usage: scale <width> <height>\n";
        return;
    }
    ImageStatus status = scratch_.scale_from(image_, size[0], size[1]);
    if (status != ImageStatus::ok) {
        std::cout << "Resize failed: " << Image::status_text(status) << '\n';
        return;
    }
    take_result();
    std::cout << "New size: " << image_.width() << " x " << image_.height() << '\n';
}

void Editor::command_fill(const std::vector<std::string>& words) {
    if (!require_image()) {
        return;
    }
    if (words.size() < 6) {
        std::cout << "Five numbers needed: fill <x> <y> <r> <g> <b>\n";
        return;
    }
    // Пять чисел подряд: координаты точки и три канала цвета.
    int v[5] = { 0, 0, 0, 0, 0 };
    if (!parse_ints(words, 1, v, 5)) {
        return;
    }
    if (!image_.inside(v[0], v[1])) {
        std::cout << "Point (" << v[0] << ", " << v[1] << ") is outside the image\n";
        return;
    }
    Color color = Color::from_rgb(Color::clamp(v[2]), Color::clamp(v[3]), Color::clamp(v[4]));
    print_fill_report(image_.flood_fill(v[0], v[1], color));
}

// Заливка из угла (0, 0) на трёх картинках. Рисуются они во вспомогательном
// изображении, чтобы не портить изображение пользователя. Шахматка: залита
// одна клетка. Градиент: залит один столбец. Одноцветная картинка: залита
// вся, и стек ни разу не вырос до числа её точек.
void Editor::command_filltest() {
    Color red = Color::from_rgb(255, 0, 0);
    std::cout << "Checker, cell 32, 256 x 256:\n";
    scratch_.reset(256, 256);
    scratch_.fill_checker(32);
    print_fill_report(scratch_.flood_fill(0, 0, red));

    std::cout << "Gradient 256 x 256:\n";
    scratch_.reset(256, 256);
    scratch_.fill_gradient();
    print_fill_report(scratch_.flood_fill(0, 0, red));

    std::cout << "Plain black image 256 x 256:\n";
    scratch_.reset(256, 256);
    print_fill_report(scratch_.flood_fill(0, 0, red));

    std::cout << "Plain black image 1024 x 1024:\n";
    scratch_.reset(1024, 1024);
    print_fill_report(scratch_.flood_fill(0, 0, red));
}

void Editor::command_minmax() const {
    if (!require_image()) {
        return;
    }
    int darkest = 0;
    int brightest = 0;
    if (!image_.min_max(darkest, brightest)) {
        std::cout << "The image is empty.\n";
        return;
    }
    std::cout << "Darkest point: " << darkest << ", brightest: " << brightest << '\n';
}

void Editor::command_load(const std::vector<std::string>& words) {
    if (words.size() < 2) {
        std::cout << "Specify the file name: load <file>\n";
        return;
    }
    const std::string& path = words[1];
    PpmFormat format = PpmFormat::text;
    ImageStatus status = scratch_.load(path, format);
    if (status != ImageStatus::ok) {
        std::cout << "Cannot read " << path << ": " << Image::status_text(status) << '\n';
        return;
    }
    take_result();
    loaded_ = true;
    path_ = path;
    std::cout << "Read: " << path << " (" << (format == PpmFormat::binary ? "binary P6" : "text P3")
              << "), size " << image_.width() << " x " << image_.height() << '\n';
}

void Editor::command_save(const std::vector<std::string>& words) {
    if (!require_image()) {
        return;
    }
    // Без аргумента изображение пишется в файл, заданный при запуске.
    std::string path = words.size() >= 2 ? words[1] : path_;
    if (path.empty()) {
        std::cout << "Specify the file name: save <file> [text|bin]\n";
        return;
    }
    PpmFormat format = PpmFormat::text;
    if (words.size() >= 3) {
        if (is_command(words[2], "bin")) {
            format = PpmFormat::binary;
        } else if (!is_command(words[2], "text")) {
            std::cout << "The kind is text or bin\n";
            return;
        }
    }
    ImageStatus status = image_.save(path, format);
    if (status != ImageStatus::ok) {
        std::cout << "Cannot write " << path << ": " << Image::status_text(status) << '\n';
        return;
    }
    path_ = path;
    std::cout << "Written: " << path << " ("
              << (format == PpmFormat::binary ? "binary P6" : "text P3") << ")\n";
}

// Таблица преобразований на краевых и обычном цвете, упаковка цвета
// в число и обратно, и что бывает с беззнаковым каналом без насыщения.
void Editor::command_colors() {
    const std::uint32_t samples[] = {
        0x000000u,   // чёрный: затемнять некуда
        0xFFFFFFu,   // белый: осветлять некуда
        0xFF0000u,   // красный: один канал на максимуме, два на нуле
        0x3C78B4u    // произвольный
    };
    std::cout << std::left << std::setw(8) << "original" << "  " << std::setw(11) << "channels"
              << "  " << std::setw(8) << "brighter" << "  " << std::setw(8) << "darker" << "  "
              << std::setw(8) << "inverted" << "  gray\n" << std::right;
    for (std::uint32_t packed : samples) {
        Color color = Color::from_packed(packed);
        Color brighter = color;   // копия: brighten меняет объект, через который вызван
        brighter.brighten(40);
        Color darker = color;
        darker.darken(40);
        print_hex(color);
        std::cout << "  " << std::setw(3) << static_cast<int>(color.red()) << ' ' << std::setw(3)
                  << static_cast<int>(color.green()) << ' ' << std::setw(3)
                  << static_cast<int>(color.blue()) << "  ";
        print_hex(brighter);
        std::cout << "  ";
        print_hex(darker);
        std::cout << "  ";
        print_hex(color.inverted());
        std::cout << "  ";
        print_hex(color.to_gray());
        std::cout << '\n';
    }

    // Упаковка и распаковка взаимно обратны: каналы возвращаются те же.
    Color round = Color::from_rgb(17, 200, 255);
    Color back = Color::from_packed(round.packed());
    std::cout << "\nPacking round trip: 17 200 255 -> ";
    print_hex(round);
    std::cout << " -> " << static_cast<int>(back.red()) << ' ' << static_cast<int>(back.green())
              << ' ' << static_cast<int>(back.blue()) << '\n';

    // Беззнаковая арифметика идёт по модулю: ниже нуля значение не уходит,
    // а «проворачивается» к верхней границе типа. Ошибкой это не считается,
    // поэтому ни компилятор, ни санитайзер ничего не скажут.
    std::cout << "\nSubtracting 40 from a channel equal to 30:\n";
    std::uint8_t narrow = 30;
    narrow = static_cast<std::uint8_t>(narrow - 40);
    std::cout << "  uint8_t without a check:      " << static_cast<int>(narrow) << '\n';
    unsigned int wide = 30u;
    wide -= 40u;
    std::cout << "  unsigned int without a check: " << wide << '\n';
    std::cout << "  int and saturation:           " << static_cast<int>(Color::clamp(30 - 40))
              << '\n';
    std::cout << "Adding 40 to a channel equal to 230:\n";
    std::cout << "  uint8_t without a check:      "
              << static_cast<int>(static_cast<std::uint8_t>(230 + 40)) << '\n';
    std::cout << "  int and saturation:           " << static_cast<int>(Color::clamp(230 + 40))
              << '\n';
}

bool Editor::execute(const std::vector<std::string>& words) {
    if (words.empty()) {
        return true;
    }
    const std::string& name = words[0];
    if (is_command(name, "info")) {
        print_info();
    } else if (is_command(name, "gen")) {
        command_gen(words);
    } else if (is_command(name, "invert")) {
        command_invert();
    } else if (is_command(name, "bright")) {
        command_bright(words);
    } else if (is_command(name, "size")) {
        command_size(words);
    } else if (is_command(name, "crop")) {
        command_crop(words);
    } else if (is_command(name, "scale")) {
        command_scale(words);
    } else if (is_command(name, "rotate")) {
        command_rotate(words);
    } else if (is_command(name, "fill")) {
        command_fill(words);
    } else if (is_command(name, "filltest")) {
        command_filltest();
    } else if (is_command(name, "minmax")) {
        command_minmax();
    } else if (is_command(name, "load")) {
        command_load(words);
    } else if (is_command(name, "save")) {
        command_save(words);
    } else if (is_command(name, "colors")) {
        command_colors();
    } else if (is_command(name, "help")) {
        print_help();
    } else if (is_command(name, "quit") || is_command(name, "exit")) {
        return false;
    } else {
        // Неизвестная операция работу не завершает: печатаем подсказку и ждём дальше.
        std::cout << "Unknown operation: " << name << '\n';
        std::cout << "Type help to see the list of operations.\n";
    }
    return true;
}
