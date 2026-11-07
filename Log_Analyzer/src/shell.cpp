#include "shell.hpp"

#include <cctype>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>

#include "log_generator.hpp"
#include "log_stats.hpp"

namespace {

// Часы для замеров: steady_clock не прыгает при переводе системного времени.
using Clock = std::chrono::steady_clock;

// Секунды с момента start с тремя знаками после точки. Флаги потока после
// вывода возвращаются: иначе все следующие дробные числа печатались бы так же.
void print_seconds_since(Clock::time_point start) {
    std::chrono::duration<double> elapsed = Clock::now() - start;
    std::ios::fmtflags flags = std::cout.flags();
    std::streamsize precision = std::cout.precision();
    std::cout << std::fixed << std::setprecision(3) << elapsed.count();
    std::cout.flags(flags);
    std::cout.precision(precision);
}

// Целое число из слова: всё слово — число. false, если это не так.
bool parse_long(const std::string& word, long& out) {
    std::istringstream in(word);
    long value = 0;
    if (!(in >> value) || !in.eof()) {
        return false;
    }
    out = value;
    return true;
}

} // namespace

void Shell::print_help() {
    std::cout << "Available commands:\n"
              << "  load <file>                - load a log\n"
              << "  bsave <file>               - save the parsed log in binary form\n"
              << "  bload <file>               - read a binary log\n"
              << "  gen <file> <count> [seed]  - create a test log\n"
              << "  summary                    - short summary of the log\n"
              << "  count                      - number of records and lines\n"
              << "  head                       - first ten records\n"
              << "  tail                       - last ten records\n"
              << "  stats                      - records by level\n"
              << "  hours                      - distribution by hour\n"
              << "  modules                    - records by module\n"
              << "  order time|level|module    - viewing order (records do not move)\n"
              << "  view [n]                   - show n records in the chosen order\n"
              << "  span                       - the earliest and the latest time\n"
              << "  sort                       - sort the records themselves by time\n"
              << "  grep <word>                - records whose text contains the word\n"
              << "  module <name>              - records of one module\n"
              << "  between <date> <time> <date> <time> - records in the interval\n"
              << "  since <date> <time>        - records from the moment on,\n"
              << "                               e.g. since 2026-03-15 12:00:00\n"
              << "  level <levels...>          - set the filter, e.g. level error fatal\n"
              << "  level all | none           - enable all levels or clear them all\n"
              << "  filter                     - show the current filter\n"
              << "  time                       - check the time packing\n"
              << "  help                       - this list\n"
              << "  quit                       - exit\n";
}

LogStatus Shell::open(const std::string& path) {
    return book_.load(path);
}

bool Shell::require_log() const {
    if (!book_.loaded()) {
        std::cout << "Log is not loaded. Use load <file> or gen.\n";
        return false;
    }
    return true;
}

// Итог выборки: сколько записей нашлось и сколько из них показано.
void Shell::print_found(long found) const {
    if (found == 0) {
        std::cout << "Nothing found.\n";
    } else if (found > LogBook::SHOW) {
        std::cout << "Records found: " << found << ", showing the first " << LogBook::SHOW
                  << ".\n";
    } else {
        std::cout << "Records found: " << found << ".\n";
    }
}

void Shell::command_load(const std::vector<std::string>& words) {
    if (words.size() < 2) {
        std::cout << "Specify the file: load <file>\n";
        return;
    }
    Clock::time_point start = Clock::now();
    LogStatus status = book_.load(words[1]);
    if (status != LogStatus::ok) {
        std::cout << "Error: " << status_text(status) << " (" << words[1] << ")\n";
        return;
    }
    std::cout << "Parsed in ";
    print_seconds_since(start);
    std::cout << " s\n";
    view_.reset();   // указатели смотрели на записи прежнего файла
    book_.print_load_report();
}

void Shell::command_bsave(const std::vector<std::string>& words) {
    if (!require_log()) {
        return;
    }
    if (words.size() < 2) {
        std::cout << "Specify the file: bsave journal.bin\n";
        return;
    }
    Clock::time_point start = Clock::now();
    LogStatus status = book_.save_binary(words[1]);
    if (status != LogStatus::ok) {
        std::cout << "Cannot save " << words[1] << ": " << status_text(status) << '\n';
        return;
    }
    std::cout << "Saved " << book_.count() << " records in ";
    print_seconds_since(start);
    std::cout << " s\n";
}

void Shell::command_bload(const std::vector<std::string>& words) {
    if (words.size() < 2) {
        std::cout << "Specify the file: bload journal.bin\n";
        return;
    }
    Clock::time_point start = Clock::now();
    LogStatus status = book_.load_binary(words[1]);
    if (status != LogStatus::ok) {
        std::cout << "Cannot read " << words[1] << ": " << status_text(status) << '\n';
        return;
    }
    view_.reset();   // указатели смотрели на записи прежнего журнала
    std::cout << "Read " << book_.count() << " records in ";
    print_seconds_since(start);
    std::cout << " s\n";
}

void Shell::command_gen(const std::vector<std::string>& words) {
    if (words.size() < 3) {
        std::cout << "Usage: gen <file> <line count> [seed]\n";
        return;
    }
    long lines = 0;
    if (!parse_long(words[2], lines) || lines <= 0) {
        std::cout << "The line count must be a positive number\n";
        return;
    }
    long seed = 1;
    if (words.size() >= 4 && (!parse_long(words[3], seed) || seed < 0)) {
        std::cout << "The seed must be a number\n";
        return;
    }
    LogGenerator generator;
    LogStatus status = generator.write(words[1], lines, static_cast<unsigned int>(seed));
    if (status != LogStatus::ok) {
        std::cout << "Cannot create " << words[1] << ": " << status_text(status) << '\n';
        return;
    }
    std::cout << "Created " << words[1] << ": " << lines << " lines, seed " << seed << '\n';
}

void Shell::command_count() {
    if (require_log()) {
        std::cout << "Records: " << book_.count() << ", lines in the file: " << book_.lines()
                  << ", passing the level filter: " << book_.count_matching(filter_) << '\n';
    }
}

void Shell::command_sort() {
    if (!require_log()) {
        return;
    }
    int depth = book_.sort_by_time();
    view_.reset();   // записи переставлены, указатели смотрят не туда
    std::cout << "Records sorted: " << book_.count() << ", max recursion depth: " << depth << '\n';
}

void Shell::command_since(const std::vector<std::string>& words) {
    if (!require_log()) {
        return;
    }
    if (words.size() < 3) {
        std::cout << "Usage: since <YYYY-MM-DD> <hh:mm:ss>\n";
        return;
    }
    Timestamp moment = Timestamp::parse(words[1], words[2]);
    if (!moment.valid()) {
        std::cout << "Cannot parse the date and time\n";
        return;
    }
    // Двоичный поиск работает только на отсортированном массиве.
    if (!book_.sorted()) {
        std::cout << "The log is not sorted, sorting.\n";
        command_sort();
    }
    long index = book_.search_not_before(moment);
    if (index == book_.count()) {
        std::cout << "No records at or after this moment.\n";
        return;
    }
    std::cout << "First matching record - number " << index + 1 << " of " << book_.count()
              << ":\n";
    for (long i = index; i < book_.count() && i < index + LogBook::SHOW; ++i) {
        book_.at(i).print();
    }
}

void Shell::command_order(const std::vector<std::string>& words) {
    if (!require_log()) {
        return;
    }
    LogView::Order order = LogView::Order::time;
    if (words.size() >= 2) {
        if (words[1] == "level") {
            order = LogView::Order::level;
        } else if (words[1] == "module") {
            order = LogView::Order::module;
        } else if (words[1] != "time") {
            std::cout << "The order is time, level or module\n";
            return;
        }
    }
    view_.sort(book_, order);
    std::cout << "Pointers sorted: " << view_.count() << ", order " << LogView::order_name(order)
              << ".\n";
    std::cout << "The records themselves did not move: head still shows them\n";
    std::cout << "in the order they were read from the file.\n";
}

void Shell::command_view(const std::vector<std::string>& words) {
    if (!require_log()) {
        return;
    }
    long n = LogBook::SHOW;
    if (words.size() >= 2 && (!parse_long(words[1], n) || n <= 0)) {
        n = LogBook::SHOW;
    }
    // Порядок ещё не выбран — по умолчанию по времени.
    if (view_.count() == 0) {
        view_.sort(book_, LogView::Order::time);
    }
    view_.print(n);
}

void Shell::command_span() const {
    if (!require_log()) {
        return;
    }
    Timestamp first;
    Timestamp last;
    if (!book_.time_range(first, last)) {
        std::cout << "No records.\n";
        return;
    }
    std::cout << "Earliest: " << first.text() << "\nLatest:   " << last.text() << '\n';
}

// Команды grep и module: одна выборка, разные условия.
void Shell::command_search(const std::vector<std::string>& words) {
    if (!require_log()) {
        return;
    }
    bool by_text = words[0] == "grep";
    if (words.size() < 2) {
        std::cout << (by_text ? "Search for what? Example: grep timeout\n"
                              : "Which module? Example: module net\n");
        return;
    }
    long found = by_text ? book_.print_containing(words[1], filter_)
                         : book_.print_module(words[1], filter_);
    print_found(found);
}

void Shell::command_between(const std::vector<std::string>& words) {
    if (!require_log()) {
        return;
    }
    if (words.size() < 5) {
        std::cout << "Usage: between <YYYY-MM-DD> <hh:mm:ss> <YYYY-MM-DD> <hh:mm:ss>\n";
        return;
    }
    Timestamp from = Timestamp::parse(words[1], words[2]);
    Timestamp to = Timestamp::parse(words[3], words[4]);
    if (!from.valid() || !to.valid()) {
        std::cout << "Cannot parse the date and time\n";
        return;
    }
    if (to.is_before(from)) {
        std::cout << "The interval starts after it ends: no records.\n";
        return;
    }
    print_found(book_.print_between(from, to, filter_));
}

void Shell::command_level(const std::vector<std::string>& words) {
    if (words.size() < 2) {
        std::cout << "level needs at least one level, all or none\n";
    } else if (words[1] == "all") {
        filter_.allow_all();
    } else if (words[1] == "none") {
        filter_.allow_none();
    } else {
        LevelFilter result;
        result.allow_none();
        for (std::size_t i = 1; i < words.size(); ++i) {
            Level level = Level::info;
            if (level_from_word(words[i], level)) {
                result.allow(level);
            } else {
                std::cout << "Unknown level: " << words[i] << '\n';
            }
        }
        // Если ни одно слово не оказалось уровнем, прежний фильтр сохраняется:
        // опечатка не должна молча скрыть весь журнал.
        if (result.empty()) {
            std::cout << "Filter unchanged.\n";
        } else {
            filter_ = result;   // объект копируется целиком, как структура в C
        }
    }
    filter_.print();
}

// Упаковка времени: порядок чисел совпадает с порядком моментов.
void Shell::command_time() {
    const int samples[4][6] = {
        { 2026, 3, 15, 23, 59, 59 },
        { 2026, 3, 16, 0, 0, 0 },
        { 2026, 12, 31, 23, 59, 59 },
        { 2027, 1, 1, 0, 0, 0 }
    };
    Timestamp previous;
    for (const int* m : samples) {
        Timestamp value = Timestamp::from_parts(m[0], m[1], m[2], m[3], m[4], m[5]);
        std::cout << std::setfill('0') << std::setw(4) << m[0] << '-' << std::setw(2) << m[1]
                  << '-' << std::setw(2) << m[2] << ' ' << std::setw(2) << m[3] << ':'
                  << std::setw(2) << m[4] << ':' << std::setw(2) << m[5] << std::setfill(' ')
                  << " -> " << value.value() << " -> " << value.text() << "  hour "
                  << value.hour() << '\n';
        if (previous.valid() && !previous.is_before(value)) {
            std::cout << "  ERROR: order is broken\n";
        }
        previous = value;
    }
    Timestamp invalid = Timestamp::from_parts(2026, 13, 1, 0, 0, 0);
    std::cout << "Invalid date 2026-13-01: " << invalid.value() << " (expected -1)\n";
}

bool Shell::execute(const std::vector<std::string>& words) {
    const std::string& name = words[0];
    if (name == "load") {
        command_load(words);
    } else if (name == "bsave") {
        command_bsave(words);
    } else if (name == "bload") {
        command_bload(words);
    } else if (name == "gen") {
        command_gen(words);
    } else if (name == "summary") {
        if (require_log()) {
            book_.print_summary();
        }
    } else if (name == "count") {
        command_count();
    } else if (name == "head" || name == "tail") {
        if (require_log()) {
            if (name == "head") {
                book_.print_head(filter_);
            } else {
                book_.print_tail(filter_);
            }
        }
    } else if (name == "stats" || name == "hours" || name == "modules") {
        if (require_log()) {
            // Статистика нужна только на время команды: локальный объект.
            LogStats stats;
            stats.collect(book_);
            if (name == "stats") {
                stats.print_levels();
            } else if (name == "hours") {
                stats.print_hours();
            } else {
                stats.print_modules();
            }
        }
    } else if (name == "order") {
        command_order(words);
    } else if (name == "view") {
        command_view(words);
    } else if (name == "span") {
        command_span();
    } else if (name == "sort") {
        command_sort();
    } else if (name == "since") {
        command_since(words);
    } else if (name == "grep" || name == "module") {
        command_search(words);
    } else if (name == "between") {
        command_between(words);
    } else if (name == "level") {
        command_level(words);
    } else if (name == "filter") {
        filter_.print();
    } else if (name == "time") {
        command_time();
    } else if (name == "help") {
        print_help();
    } else if (name == "quit") {
        return false;
    } else {
        std::cout << "Unknown command: " << name << '\n';
        std::cout << "Type help for the list of commands.\n";
    }
    return true;
}

void Shell::run() {
    if (book_.loaded()) {
        book_.print_summary();
    } else {
        std::cout << "Log is not loaded.\n";
    }
    std::cout << "Type help for the list of commands.\n";

    // Конец ввода и команда quit сводятся к одному флагу: у цикла одно
    // условие выхода, без break и continue.
    bool running = true;
    while (running) {
        std::cout << "> " << std::flush;
        std::string line;
        if (!std::getline(std::cin, line)) {
            running = false;
        } else {
            // Слова разделяются пробелами. К нижнему регистру приводится
            // только имя команды: остальные слова могут быть именами файлов.
            std::istringstream stream(line);
            std::vector<std::string> words;
            std::string word;
            while (stream >> word) {
                words.push_back(word);
            }
            if (!words.empty()) {
                for (char& c : words[0]) {
                    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                }
                running = execute(words);
            }
        }
    }
    std::cout << "Done.\n";
}
