#include "log_view.hpp"

const char* LogView::order_name(Order order) {
    switch (order) {
    case Order::time:
        return "by time";
    case Order::level:
        return "by level";
    case Order::module:
        return "by module";
    }
    return "unknown order";
}

int LogView::compare(const LogRecord* a, const LogRecord* b) const {
    if (order_ == Order::level && a->level() != b->level()) {
        return static_cast<int>(a->level()) - static_cast<int>(b->level());
    }
    if (order_ == Order::module) {
        int result = a->module().compare(b->module());
        if (result != 0) {
            return result;
        }
    }
    if (a->is_earlier_than(*b)) {
        return -1;
    }
    return b->is_earlier_than(*a) ? 1 : 0;
}

void LogView::merge(std::size_t left, std::size_t middle, std::size_t right) {
    std::size_t i = left;
    std::size_t j = middle;
    std::size_t k = left;
    while (i < middle && j < right) {
        if (compare(items_[i], items_[j]) <= 0) {
            buffer_[k] = items_[i];
            ++i;
        } else {
            buffer_[k] = items_[j];
            ++j;
        }
        ++k;
    }
    while (i < middle) {
        buffer_[k] = items_[i];
        ++i;
        ++k;
    }
    while (j < right) {
        buffer_[k] = items_[j];
        ++j;
        ++k;
    }
    // Копируются указатели, а не записи.
    for (std::size_t n = left; n < right; ++n) {
        items_[n] = buffer_[n];
    }
}

void LogView::merge_sort(std::size_t left, std::size_t right) {
    if (right - left < 2) {
        return;
    }
    std::size_t middle = left + (right - left) / 2;
    merge_sort(left, middle);
    merge_sort(middle, right);
    merge(left, middle, right);
}

void LogView::sort(const LogBook& book, Order order) {
    order_ = order;
    items_.clear();
    for (long i = 0; i < book.count(); ++i) {
        items_.push_back(&book.at(i));   // адрес записи, а не её копия
    }
    buffer_.resize(items_.size());
    merge_sort(0, items_.size());
}

void LogView::print(long n) const {
    for (long i = 0; i < n && i < count(); ++i) {
        items_[static_cast<std::size_t>(i)]->print();
    }
}
