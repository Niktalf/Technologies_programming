#include "log_book.hpp"

#include <cctype>
#include <iostream>
#include <stdexcept>
#include <utility>

namespace {

bool same_char(char a, char b) {
    return std::tolower(static_cast<unsigned char>(a))
           == std::tolower(static_cast<unsigned char>(b));
}

// Есть ли needle в haystack без учёта регистра. Тот же перебор позиций,
// что text_find_ignore_case в версии на C.
bool contains_ignore_case(const std::string& haystack, const std::string& needle) {
    bool found = needle.empty();
    // Условие цикла нестрогое: последнее вхождение начинается ровно
    // в позиции size() - needle.size().
    for (std::size_t start = 0; !found && start + needle.size() <= haystack.size(); ++start) {
        std::size_t i = 0;
        while (i < needle.size() && same_char(haystack[start + i], needle[i])) {
            ++i;
        }
        found = i == needle.size();
    }
    return found;
}

bool same_word_ignore_case(const std::string& a, const std::string& b) {
    if (a.size() != b.size()) {
        return false;
    }
    bool same = true;
    for (std::size_t i = 0; i < a.size() && same; ++i) {
        same = same_char(a[i], b[i]);
    }
    return same;
}

} // namespace

LogBook::LogBook(std::string path, std::vector<LogRecord> records, long lines, long broken)
    : records_(std::move(records)), path_(std::move(path)), lines_(lines), broken_(broken),
      loaded_(true) {
    if (broken_ < 0 || count() + broken_ > lines_) {
        throw std::invalid_argument("the line counts do not add up");
    }
}

void LogBook::print_load_report() const {
    std::cout << "File: " << path_ << '\n';
    std::cout << "Lines: " << lines_ << ", records: " << count() << '\n';
    if (broken_ > 0) {
        std::cout << "Lines that could not be parsed: " << broken_ << '\n';
    }
}

void LogBook::print_summary() const {
    print_load_report();
    if (records_.empty()) {
        return;
    }
    long per_level[LEVEL_COUNT] = {};
    std::size_t first = 0;
    std::size_t last = 0;
    for (std::size_t i = 0; i < records_.size(); ++i) {
        ++per_level[static_cast<int>(records_[i].level())];
        if (records_[i].is_earlier_than(records_[first])) {
            first = i;
        }
        if (records_[last].is_earlier_than(records_[i])) {
            last = i;
        }
    }
    std::cout << "By level:";
    for (int i = 0; i < LEVEL_COUNT; ++i) {
        std::cout << ' ' << level_name(static_cast<Level>(i)) << ' ' << per_level[i];
    }
    std::cout << "\nEarliest record: " << records_[first].time().text() << '\n';
    std::cout << "Latest record:   " << records_[last].time().text() << '\n';
}

void LogBook::print_head(const LevelFilter& filter) const {
    int printed = 0;
    for (std::size_t i = 0; i < records_.size() && printed < SHOW; ++i) {
        if (filter.allows(records_[i].level())) {
            records_[i].print();
            ++printed;
        }
    }
}

void LogBook::print_tail(const LevelFilter& filter) const {
    // Идём с конца, пока не наберём SHOW подходящих записей, — так находим,
    // с какой записи начинать. Печатаем уже в прямом порядке.
    std::size_t first = records_.size();
    int found = 0;
    while (first > 0 && found < SHOW) {
        --first;
        if (filter.allows(records_[first].level())) {
            ++found;
        }
    }
    for (std::size_t i = first; i < records_.size(); ++i) {
        if (filter.allows(records_[i].level())) {
            records_[i].print();
        }
    }
}

long LogBook::count_matching(const LevelFilter& filter) const {
    long matching = 0;
    for (const LogRecord& record : records_) {
        if (filter.allows(record.level())) {
            ++matching;
        }
    }
    return matching;
}

// Один цикл на все выборки — вместо трёх одинаковых, как print_matching
// в версии на C. Там условие передавалось указателем на функцию; здесь
// выборка называется значением перечисления, а проверка — в одном месте.
long LogBook::print_matching(Selection selection, const std::string& word, Timestamp from,
                             Timestamp to, const LevelFilter& filter) const {
    long found = 0;
    for (const LogRecord& record : records_) {
        bool matches = false;
        if (selection == Selection::text) {
            matches = contains_ignore_case(record.text(), word);
        } else if (selection == Selection::module) {
            matches = same_word_ignore_case(record.module(), word);
        } else {
            matches = !record.time().is_before(from) && !to.is_before(record.time());
        }
        if (filter.allows(record.level()) && matches) {
            if (found < SHOW) {
                record.print();
            }
            ++found;
        }
    }
    return found;
}

long LogBook::print_containing(const std::string& word, const LevelFilter& filter) const {
    return print_matching(Selection::text, word, Timestamp(), Timestamp(), filter);
}

long LogBook::print_module(const std::string& module, const LevelFilter& filter) const {
    return print_matching(Selection::module, module, Timestamp(), Timestamp(), filter);
}

long LogBook::print_between(Timestamp from, Timestamp to, const LevelFilter& filter) const {
    return print_matching(Selection::interval, "", from, to, filter);
}

bool LogBook::time_range(Timestamp& first, Timestamp& last) const {
    if (records_.empty()) {
        return false;
    }
    // Начальные значения берём из первой записи, а не из нуля и не из
    // максимума: так проверка работает при любых данных.
    first = records_[0].time();
    last = first;
    for (const LogRecord& record : records_) {
        if (record.time().is_before(first)) {
            first = record.time();
        }
        if (last.is_before(record.time())) {
            last = record.time();
        }
    }
    return true;
}

// Сливает две отсортированные половины [left, middle) и [middle, right).
// Результат пишется во временный массив, а не туда, откуда читаем,
// иначе ещё не прочитанные записи будут затёрты.
void LogBook::merge(std::size_t left, std::size_t middle, std::size_t right) {
    std::size_t i = left;
    std::size_t j = middle;
    std::size_t k = left;
    while (i < middle && j < right) {
        // Нестрогое сравнение сохраняет устойчивость: при равном времени
        // первой идёт запись из левой половины.
        if (!records_[j].is_earlier_than(records_[i])) {
            buffer_[k] = records_[i];
            ++i;
        } else {
            buffer_[k] = records_[j];
            ++j;
        }
        ++k;
    }
    // Одна из половин кончилась; остаток другой уже упорядочен.
    while (i < middle) {
        buffer_[k] = records_[i];
        ++i;
        ++k;
    }
    while (j < right) {
        buffer_[k] = records_[j];
        ++j;
        ++k;
    }
    for (std::size_t n = left; n < right; ++n) {
        records_[n] = buffer_[n];
    }
}

void LogBook::merge_sort(std::size_t left, std::size_t right, int depth, int& max_depth) {
    if (depth > max_depth) {
        max_depth = depth;
    }
    // Условие остановки: ноль или один элемент уже отсортированы.
    if (right - left < 2) {
        return;
    }
    std::size_t middle = left + (right - left) / 2;
    merge_sort(left, middle, depth + 1, max_depth);
    merge_sort(middle, right, depth + 1, max_depth);
    merge(left, middle, right);
}

int LogBook::sort_by_time() {
    // Временный массив — копия записей, а не resize: resize создал бы
    // «пустые» записи конструктором по умолчанию, а такого у записи больше
    // нет — пустая запись нарушала бы её инвариант. Копия сразу нужного
    // размера, и все её элементы — верные записи.
    buffer_ = records_;
    int max_depth = 0;
    merge_sort(0, records_.size(), 1, max_depth);
    sorted_ = true;
    return max_depth;
}

// Ищет в полуинтервале [low, high). Инвариант: все записи левее low
// раньше moment, все записи начиная с high — не раньше.
long LogBook::lower_bound(long low, long high, Timestamp moment) const {
    // Диапазон пуст — граница найдена. Без этого условия поиск
    // зацикливается на двух элементах.
    if (low >= high) {
        return low;
    }
    // Середина без переполнения: (low + high) / 2 переполнилось бы
    // на больших номерах.
    long middle = low + (high - low) / 2;
    if (records_[static_cast<std::size_t>(middle)].time().is_before(moment)) {
        return lower_bound(middle + 1, high, moment);
    }
    // middle может оказаться ответом, поэтому он остаётся в диапазоне.
    return lower_bound(low, middle, moment);
}

long LogBook::search_not_before(Timestamp moment) const {
    return lower_bound(0, count(), moment);
}
