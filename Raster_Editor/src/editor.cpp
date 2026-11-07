#include "editor.hpp"

#include <cctype>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <utility>

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

// Изображение передаётся по значению: внутри функции живёт копия.
// Её создаёт копирующий конструктор при входе, уничтожает деструктор
// при выходе. Оригинал не меняется.
void show_by_value(Image copy) {
    std::cout << "  inside the function, the copy:\n";
    copy.set_pixel(0, 0, Color::from_rgb(255, 0, 0));
    copy.print_short();
}

// Возвращает изображение по значению. Копии при возврате нет: компилятор
// строит результат прямо там, куда его присваивают.
Image make_gradient(int width, int height) {
    Image result(width, height);
    result.fill_gradient();
    return result;
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
              << "  demo                     - object lifetime: copies and moves\n"
              << "  stress [n]               - create, copy, move and destroy n times\n"
              << "  help                     - this list\n"
              << "  quit                     - exit\n";
}

bool Editor::require_image() const {
    if (image_.empty()) {
        std::cout << "No image. Create one with gen.\n";
        return false;
    }
    return true;
}

bool Editor::ensure_image(int width, int height) {
    if (!image_.empty() && image_.width() == width && image_.height() == height) {
        return true;
    }
    // Новое изображение создаётся раньше, чем освобождается старое: если
    // размер неверный, конструктор бросит исключение, и прежнее останется.
    try {
        image_ = Image(width, height);   // временный объект перемещается в image_
    } catch (const std::invalid_argument& error) {
        std::cout << "Cannot create an image " << width << " x " << height << ": "
                  << error.what() << '\n';
        return false;
    }
    return true;
}

void Editor::print_info() const {
    std::cout << "File: " << (path_.empty() ? "not set" : path_) << '\n';
    if (image_.empty()) {
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
    if (image_.crop(v[0], v[1], v[2], v[3]) != ImageStatus::ok) {
        std::cout << "Crop failed: the rectangle must lie inside the image.\n";
        return;
    }
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
    if (image_.rotate(angle) != ImageStatus::ok) {
        std::cout << "The angle must be a multiple of 90 degrees.\n";
        return;
    }
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
    ImageStatus status = image_.scale(size[0], size[1]);
    if (status != ImageStatus::ok) {
        std::cout << "Resize failed: " << Image::status_text(status) << '\n';
        return;
    }
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

// Заливка из угла (0, 0) на трёх картинках. Каждая — отдельный локальный
// объект: изображение пользователя не трогается, а память картинки
// освобождается сама в конце её блока. Шахматка: залита одна клетка.
// Градиент: залит один столбец. Одноцветная картинка: залита вся.
void Editor::command_filltest() {
    Color red = Color::from_rgb(255, 0, 0);
    {
        std::cout << "Checker, cell 32, 256 x 256:\n";
        Image sample(256, 256);
        sample.fill_checker(32);
        print_fill_report(sample.flood_fill(0, 0, red));
    }
    {
        std::cout << "Gradient 256 x 256:\n";
        Image sample(256, 256);
        sample.fill_gradient();
        print_fill_report(sample.flood_fill(0, 0, red));
    }
    {
        std::cout << "Plain black image 256 x 256:\n";
        Image sample(256, 256);
        print_fill_report(sample.flood_fill(0, 0, red));
    }
    {
        std::cout << "Plain black image 2048 x 2048:\n";
        Image sample(2048, 2048);
        print_fill_report(sample.flood_fill(0, 0, red));
    }
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
    // load читает во временное изображение и перемещает его в image_,
    // только если файл прочитан целиком: оборванный файл не портит открытое.
    ImageStatus status = image_.load(path, format);
    if (status != ImageStatus::ok) {
        std::cout << "Cannot read " << path << ": " << Image::status_text(status) << '\n';
        return;
    }
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

// Опыты со временем жизни. Image::trace включает сообщения из конструкторов,
// деструктора, копий и перемещений; Image::live_count показывает, сколько
// изображений владеют буфером.
void Editor::command_demo() {
    Image::trace = true;

    // Все объекты опытов живут в этом блоке, чтобы сообщения их деструкторов
    // появились до того, как trace выключится.
    {

        std::cout << "1. A stack object is destroyed when the block ends:\n";
        {
            Image image(4, 3);
            std::cout << "  inside the block, images: " << Image::live_count() << '\n';
        }   // здесь вызывается деструктор image
        std::cout << "  after the block, images: " << Image::live_count() << "\n\n";

        std::cout << "2. Pass by value - a copy is created and destroyed:\n";
        Image original(64, 48);
        original.fill_gradient();
        show_by_value(original);
        std::cout << "  after the function the original is unchanged:\n";
        original.print_short();
        std::cout << '\n';

        std::cout << "3. The copy is independent of the original:\n";
        Image copy = original;   // это копирующий конструктор, а не присваивание
        copy.set_pixel(0, 0, Color::from_rgb(0, 255, 0));
        copy.print_short();
        original.print_short();
        std::cout << '\n';

        std::cout << "4. Copy assignment and self-assignment:\n";
        Image other(2, 2);
        other = original;
        other.print_short();
        Image& same = other;   // через ссылку: прямое other = other компилятор считает опечаткой
        other = same;
        std::cout << '\n';

        std::cout << "5. Moves: the buffer changes owner, nothing is copied:\n";
        Image made = make_gradient(32, 16);   // возврат по значению: ни копии, ни перемещения
        Image moved = std::move(made);        // перемещающий конструктор
        std::cout << "  the source after the move:\n";
        made.print_short();
        moved = Image(8, 8);                  // временный объект перемещается в moved
        moved.print_short();
        std::cout << '\n';

        std::cout << "6. Invalid size - no object is created:\n";
        try {
            Image broken(0, 100);
        } catch (const std::exception& error) {
            std::cout << "  caught: " << error.what() << '\n';
        }
    }   // здесь деструкторы moved, made, other, copy, original — в обратном порядке
    std::cout << "End of demo: the objects were destroyed in reverse order of creation.\n";
    Image::trace = false;
}

// Создаёт, копирует, перемещает и уничтожает изображения times раз.
// Если что-то не освобождается, потребление памяти растёт с каждым кругом.
void Editor::command_stress(const std::vector<std::string>& words) {
    int times = 100;
    if (words.size() >= 2 && (!parse_int(words[1], times) || times <= 0)) {
        std::cout << "stress needs a positive number, e.g. stress 100\n";
        return;
    }
    Image::trace = false;
    for (int i = 0; i < times; ++i) {
        Image image(512, 512);
        image.fill_gradient();
        Image copy(image);                 // копирующий конструктор
        Image other(16, 16);
        other = copy;                      // копирующее присваивание
        Image taken = std::move(other);    // перемещение
        taken.set_pixel(0, 0, Color::from_rgb(1, 2, 3));
        copy.rotate(90);                   // результат перемещается на место copy
    }                                      // четыре деструктора на каждом круге
    std::cout << "Cycles: " << times << ", images now: " << Image::live_count() << '\n';
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
    } else if (is_command(name, "demo")) {
        command_demo();
    } else if (is_command(name, "stress")) {
        command_stress(words);
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
