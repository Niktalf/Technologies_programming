#include "log_book.hpp"

#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <utility>

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

// Практика 14: сводка без циклов с накопителем. Число записей уровня —
// count_if (filter + reduce); первая и последняя по времени — minmax_element
// за один проход. На практике 13 всё это делал один цикл с массивом
// счётчиков и двумя номерами.
void LogBook::print_summary() const {
    print_load_report();
    if (records_.empty()) {
        return;
    }
    std::cout << "By level:";
    for (int i = 0; i < LEVEL_COUNT; ++i) {
        Level level = static_cast<Level>(i);
        long per_level = std::count_if(records_.begin(), records_.end(),
                                       [level](const LogRecord& r) { return r.level() == level; });
        std::cout << ' ' << level_name(level) << ' ' << per_level;
    }
    Timestamp first;
    Timestamp last;
    time_range(first, last);
    std::cout << "\nEarliest record: " << first.text() << '\n';
    std::cout << "Latest record:   " << last.text() << '\n';
}

bool LogBook::time_range(Timestamp& first, Timestamp& last) const {
    if (records_.empty()) {
        return false;
    }
    // Самая ранняя и самая поздняя запись одним проходом. Начального
    // значения у minmax_element нет вовсе: он начинает с первой записи, и
    // ошибиться с ним, как с нулём в начале поиска минимума, нельзя.
    auto range = std::minmax_element(records_.begin(), records_.end(),
                                      [](const LogRecord& a, const LogRecord& b) {
                                          return a.is_earlier_than(b);
                                      });
    first = range.first->time();
    last = range.second->time();
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
