#pragma once

#include <vector>

#include "log_book.hpp"

// Порядок просмотра: указатели на записи журнала, отсортированные
// по выбранному ключу. Сами записи не двигаются — как на практике 5.
//
// Указатели смотрят внутрь вектора журнала. Когда журнал перечитан или
// отсортирован, вектор переложен, и указатели больше не годятся: тогда
// вид сбрасывается методом reset.
class LogView {
public:
    enum class Order { time, level, module };

    static const char* order_name(Order order);

    // Строит порядок для всех записей журнала. Сортировка слиянием,
    // как у журнала, только переставляются указатели — восемь байт
    // вместо целой записи.
    void sort(const LogBook& book, Order order);

    void reset() { items_.clear(); }
    long count() const { return static_cast<long>(items_.size()); }

    // Печатает первые n записей в выбранном порядке.
    void print(long n) const;

private:
    // Сравнение по ключу; при равном ключе — по времени, иначе порядок
    // внутри уровня или модуля был бы случайным. Отрицательное, ноль или
    // положительное — как strcmp.
    int compare(const LogRecord* a, const LogRecord* b) const;
    void merge_sort(std::size_t left, std::size_t right);
    void merge(std::size_t left, std::size_t middle, std::size_t right);

    std::vector<const LogRecord*> items_;
    std::vector<const LogRecord*> buffer_;
    Order order_ = Order::time;
};
