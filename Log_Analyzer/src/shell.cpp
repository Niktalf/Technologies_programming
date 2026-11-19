#include "shell.hpp"

#include <cctype>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <utility>

#include "binary_log.hpp"
#include "file_reader.hpp"
#include "log_parser.hpp"
#include "log_report.hpp"
#include "record_filter.hpp"
#include "log_source.hpp"
#include "log_generator.hpp"

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
              << "  modules                    - top modules by number of records\n"
              << "  cond <condition>           - save a condition: level <levels>, module <m>,\n"
              << "                               grep <word>, between <d> <t> <d> <t> or any\n"
              << "  match [from <source>]      - records matching the saved condition\n"
              << "  report <what> from <source> - reports on any source of records:\n"
              << "      what:   levels, modules, hours or all, e.g. report levels hours\n"
              << "      source: book | file <path> | files <path> <path> ... |\n"
              << "              random <count> [seed]\n"
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
              << "  demo                       - object lifetime, an exception, invariants\n"
              << "  stress [n]                 - load and destroy the log n times\n"
              << "  help                       - this list\n"
              << "  quit                       - exit\n";
}

void Shell::open(const std::string& path) {
    // Временный журнал перемещается в book_: векторы записей переезжают,
    // а не копируются.
    book_ = LogParser::read(path);
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
    // Новый журнал строится отдельным объектом. Если файл не открылся,
    // LogParser::read бросит исключение, и прежний журнал останется —
    // в версии на C он к этому моменту был бы уже очищен.
    try {
        book_ = LogParser::read(words[1]);
    } catch (const LogError& error) {
        std::cout << "Error: " << error.what() << '\n';
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
    LogStatus status = BinaryLog::write(book_, words[1]);
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
    try {
        // BinaryLog::read возвращает журнал по значению, и он перемещается в book_.
        book_ = BinaryLog::read(words[1]);
    } catch (const LogError& error) {
        std::cout << "Cannot read " << words[1] << ": " << status_text(error.status()) << '\n';
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
                  << ", passing the level filter: " << count_matching(book_, by_levels(filter_))
                  << '\n';
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

// Выборка по условию и фильтру уровней: первые SHOW записей и сколько
// нашлось всего. Условие склеено с фильтром уровней функцией both.
void Shell::show_selected(const Predicate& condition) const {
    std::vector<const LogRecord*> found = select(book_, both(by_levels(filter_), condition));
    for (std::size_t i = 0; i < found.size() && i < static_cast<std::size_t>(LogBook::SHOW); ++i) {
        found[i]->print();
    }
    print_found(static_cast<long>(found.size()));
}

// head и tail: первые или последние SHOW записей, прошедших фильтр уровней.
void Shell::command_head_tail(bool head) const {
    std::vector<const LogRecord*> found = select(book_, by_levels(filter_));
    std::size_t shown = static_cast<std::size_t>(LogBook::SHOW);
    std::size_t first = head || found.size() <= shown ? 0 : found.size() - shown;
    for (std::size_t i = first; i < found.size() && i < first + shown; ++i) {
        found[i]->print();
    }
}

// Условие по словам: level <уровни> | module <имя> | grep <слово> |
// between <дата> <время> <дата> <время> | any. Удалось — true, условие
// в out, его описание в text.
bool Shell::parse_condition(const std::vector<std::string>& words, Predicate& out,
                            std::string& text) {
    const std::string& kind = words[1];
    if (kind == "any" && words.size() == 2) {
        out = any_record();
        text = "any record";
        return true;
    }
    if ((kind == "module" || kind == "grep") && words.size() == 3) {
        out = kind == "module" ? by_module(words[2]) : by_text(words[2]);
        text = kind + " " + words[2];
        return true;
    }
    if (kind == "between" && words.size() == 6) {
        Timestamp from = Timestamp::parse(words[2], words[3]);
        Timestamp to = Timestamp::parse(words[4], words[5]);
        if (from.valid() && to.valid()) {
            out = between(from, to);
            text = "between " + from.text() + " and " + to.text();
            return true;
        }
    }
    if (kind == "level" && words.size() >= 3) {
        LevelFilter levels;
        levels.allow_none();
        text = "level";
        for (std::size_t i = 2; i < words.size(); ++i) {
            Level level = Level::info;
            if (!level_from_word(words[i], level)) {
                return false;
            }
            levels.allow(level);
            text += std::string(" ") + level_name(level);
        }
        out = by_levels(levels);
        return true;
    }
    return false;
}

// cond <условие> — сохранить условие в переменную saved_; cond — показать.
// Сохранённое условие живёт, пока его не заменят, и применяется командой
// match к журналу или к любому источнику.
void Shell::command_cond(const std::vector<std::string>& words) {
    if (words.size() >= 2) {
        Predicate condition;
        std::string text;
        if (!parse_condition(words, condition, text)) {
            std::cout << "Usage: cond level <levels> | module <name> | grep <word> |\n"
                      << "            between <date> <time> <date> <time> | any\n";
            return;
        }
        // Присваивание std::function: старое условие уничтожается, новое
        // — копия лямбды со всем, что она захватила, — занимает его место.
        saved_ = condition;
        saved_text_ = text;
    }
    std::cout << "Saved condition: " << saved_text_ << '\n';
}

// match — записи загруженного журнала по сохранённому условию;
// match from <источник> — сколько их в любом источнике.
void Shell::command_match(const std::vector<std::string>& words) {
    if (words.size() == 1) {
        if (require_log()) {
            std::cout << "Condition: " << saved_text_ << '\n';
            // Фильтр уровней тоже действует: условие склеивается с ним.
            show_selected(saved_);
        }
        return;
    }
    if (words[1] != "from") {
        std::cout << "Usage: match or match from <source>\n";
        return;
    }
    try {
        std::unique_ptr<LogSource> source = make_source(words, 2);
        if (!source) {
            return;
        }
        // Сохранённое условие уходит в отчёт, а отчёт — в тот же цикл
        // run_reports, что и у команды report.
        std::vector<std::unique_ptr<LogReport>> reports;
        reports.push_back(std::make_unique<MatchReport>(saved_, saved_text_));
        long count = run_reports(*source, reports);
        reports[0]->print();
        std::cout << "Records: " << count << ", source: " << source->describe() << '\n';
    } catch (const LogError& error) {
        std::cout << "Error: " << error.what() << '\n';
    }
}

// Команды grep и module: одна выборка, разные условия.
void Shell::command_search(const std::vector<std::string>& words) {
    if (!require_log()) {
        return;
    }
    bool text_search = words[0] == "grep";
    if (words.size() < 2) {
        std::cout << (text_search ? "Search for what? Example: grep timeout\n"
                              : "Which module? Example: module net\n");
        return;
    }
    show_selected(text_search ? by_text(words[1]) : by_module(words[1]));
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
    show_selected(between(from, to));
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

// Опыты со временем жизни на файле загруженного журнала.
void Shell::command_demo() {
    if (!require_log()) {
        return;
    }
    const std::string& path = book_.path();
    FileReader::set_trace(true);

    std::cout << "1. A stack object is destroyed when the block ends:\n";
    {
        FileReader reader(path);
        std::cout << "  inside the block, open files: " << FileReader::open_count() << '\n';
    }   // здесь деструктор reader закрывает файл
    std::cout << "  after the block, open files: " << FileReader::open_count() << "\n\n";

    std::cout << "2. The log is read by LogParser; the file is closed before\n"
              << "   read returns, and the log lives on:\n";
    {
        LogBook book = LogParser::read(path);
        std::cout << "  log created, records: " << book.count()
                  << ", open files: " << FileReader::open_count() << '\n';
    }
    std::cout << '\n';

    std::cout << "3. An exception in the middle of reading:\n";
    try {
        LogParser::read_until_failure(path, 3);
    } catch (const std::exception& error) {
        std::cout << "  caught: " << error.what() << '\n';
    }
    std::cout << "  open files: " << FileReader::open_count()
              << " - the destructor closed the file during stack unwinding\n\n";

    std::cout << "4. No file - no log object is created:\n";
    try {
        LogBook book = LogParser::read("no-such-file.log");
        std::cout << "  this line is never printed\n";
    } catch (const std::exception& error) {
        std::cout << "  caught: " << error.what() << '\n';
    }
    std::cout << "  open files: " << FileReader::open_count() << "\n\n";

    // Владение файлом можно передать: перемещение отдаёт FILE * другому
    // объекту, и закроет файл ровно один деструктор.
    std::cout << "5. Moving the owner: the file is closed exactly once:\n";
    {
        FileReader first(path);
        FileReader second = std::move(first);
        std::cout << "  after the move, open files: " << FileReader::open_count() << '\n';
    }
    std::cout << "  after the block, open files: " << FileReader::open_count() << "\n\n";

    FileReader::set_trace(false);
    try_to_break();
}

// Практика 11: попытки создать запись и журнал в неверном состоянии.
// Каждая отвергнута конструктором — проверять это у вызывающего не нужно.
void Shell::try_to_break() {
    std::cout << "6. Trying to break the invariants from outside:\n";
    Timestamp moment = Timestamp::parse("2026-03-15 10:30:00");
    struct Attempt {
        const char* what;
        Timestamp time;
        Level level;
        const char* module;
    };
    const Attempt attempts[] = {
        { "empty module", moment, Level::info, "" },
        { "level number 7", moment, static_cast<Level>(7), "net" },
        { "invalid time", Timestamp(), Level::error, "net" },
        { "year 1500", Timestamp::from_parts(1500, 1, 1, 0, 0, 0), Level::warn, "net" },
    };
    for (const Attempt& attempt : attempts) {
        try {
            LogRecord record(attempt.time, attempt.level, attempt.module, "text");
            std::cout << "  " << attempt.what << ": created - the invariant is broken!\n";
        } catch (const std::invalid_argument& error) {
            std::cout << "  " << attempt.what << ": rejected - " << error.what() << '\n';
        }
    }
    Timestamp far = Timestamp::from_value(1000000000000000LL);
    std::cout << "  time from the number 10^15: " << (far.valid() ? "valid" : "invalid") << '\n';
    try {
        LogBook book("x.log", std::vector<LogRecord>(), 3, 5);
        std::cout << "  a log with 5 broken lines out of 3: created - the invariant is broken!\n";
    } catch (const std::invalid_argument& error) {
        std::cout << "  a log with 5 broken lines out of 3: rejected - " << error.what() << '\n';
    }
}

// Загружает и уничтожает журнал times раз. Память записей освобождает
// деструктор вектора, файл закрывает деструктор FileReader.
void Shell::command_stress(const std::vector<std::string>& words) {
    if (!require_log()) {
        return;
    }
    long times = 20;
    if (words.size() >= 2 && (!parse_long(words[1], times) || times <= 0)) {
        std::cout << "stress needs a positive number, e.g. stress 100\n";
        return;
    }
    long records = 0;
    for (long i = 0; i < times; ++i) {
        LogBook book = LogParser::read(book_.path());
        records = book.count();
    }
    std::cout << "Logs loaded and destroyed: " << times << ", records in each: " << records
              << '\n';
    std::cout << "Open files after all cycles: " << FileReader::open_count() << '\n';
}

// Отчёты stats, hours и modules по загруженному журналу. Источник —
// загруженный журнал, отчёт — один. Тот же цикл run_reports, что у report.
void Shell::command_stats(const std::string& name) const {
    const char* report_name = name == "stats" ? "levels" : name.c_str();
    std::vector<std::unique_ptr<LogReport>> reports;
    reports.push_back(make_report(report_name));
    BookSource source(book_);
    run_reports(source, reports);
    reports[0]->print();
}

// Источник по словам начиная с words[position]: book, file <путь>,
// files <путь> ..., random <число> [зерно]. Неверно — сообщение и пустой
// указатель. Файл не открылся — LogError.
std::unique_ptr<LogSource> Shell::make_source(const std::vector<std::string>& words,
                                              std::size_t position) const {
    if (position >= words.size()) {
        std::cout << "Which source? book, file, files or random\n";
        return nullptr;
    }
    const std::string& kind = words[position];
    std::size_t rest = words.size() - position - 1;   // слов после вида источника
    if (kind == "book" && rest == 0) {
        if (!require_log()) {
            return nullptr;
        }
        return std::make_unique<BookSource>(book_);
    }
    if (kind == "file" && rest == 1) {
        return open_file_source(words[position + 1]);
    }
    if (kind == "files" && rest >= 1) {
        // Источник из нескольких файлов содержит источники. Каждый файл
        // может быть и текстовым, и двоичным.
        std::unique_ptr<MultiFileSource> multi = std::make_unique<MultiFileSource>();
        for (std::size_t i = position + 1; i < words.size(); ++i) {
            multi->add(open_file_source(words[i]));
        }
        return multi;
    }
    long count = 0;
    long seed = 1;
    if (kind == "random" && (rest == 1 || rest == 2) && parse_long(words[position + 1], count)
        && count >= 0 && (rest == 1 || (parse_long(words[position + 2], seed) && seed >= 0))) {
        return std::make_unique<RandomSource>(count, static_cast<unsigned int>(seed));
    }
    std::cout << "Unknown source. Use: book | file <path> | files <path> ... |"
              << " random <count> [seed]\n";
    return nullptr;
}

// report levels hours from files a.log b.bin
// Любой набор отчётов по любому источнику. Сначала собираются отчёты
// и источник, потом run_reports гонит записи — не зная, кто есть кто.
void Shell::command_report(const std::vector<std::string>& words) {
    std::size_t from = 1;
    while (from < words.size() && words[from] != "from") {
        ++from;
    }
    if (from == 1 || from == words.size()) {
        std::cout << "Usage: report <levels|modules|hours|all ...> from <source>\n";
        return;
    }
    std::vector<std::unique_ptr<LogReport>> reports;
    for (std::size_t i = 1; i < from; ++i) {
        if (words[i] == "all") {
            reports.push_back(make_report("levels"));
            reports.push_back(make_report("modules"));
            reports.push_back(make_report("hours"));
        } else {
            std::unique_ptr<LogReport> report = make_report(words[i]);
            if (!report) {
                std::cout << "Unknown report: " << words[i] << ". Known: levels, modules, hours\n";
                return;
            }
            reports.push_back(std::move(report));
        }
    }
    try {
        std::unique_ptr<LogSource> source = make_source(words, from + 1);
        if (!source) {
            return;
        }
        long count = run_reports(*source, reports);
        for (const std::unique_ptr<LogReport>& report : reports) {
            std::cout << "== " << report->title() << " ==\n";
            report->print();
        }
        std::cout << "Records: " << count << ", source: " << source->describe() << '\n';
    } catch (const LogError& error) {
        std::cout << "Error: " << error.what() << '\n';
    }
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
            command_head_tail(name == "head");
        }
    } else if (name == "cond") {
        command_cond(words);
    } else if (name == "match") {
        command_match(words);
    } else if (name == "stats" || name == "hours" || name == "modules") {
        if (require_log()) {
            command_stats(name);
        }
    } else if (name == "report") {
        command_report(words);
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
    } else if (name == "demo") {
        command_demo();
    } else if (name == "stress") {
        command_stress(words);
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
