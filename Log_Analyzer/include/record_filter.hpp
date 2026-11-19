#pragma once

#include <functional>
#include <string>
#include <vector>

#include "level.hpp"
#include "log_book.hpp"
#include "log_record.hpp"
#include "timestamp.hpp"

// Практика 13: условие отбора как значение.
//
// Условие — функция, которая получает запись и отвечает, подходит ли она.
// Её можно передать в функцию, положить в переменную, собрать из двух
// других. На практике 12 у журнала было три метода выборки и перечисление
// Selection, чтобы они обошлись одним циклом; добавить четвёртую выборку
// значило бы добавить в перечисление значение и в цикл ветку. Теперь
// четвёртая выборка — одна строка: новая лямбда.
//
// Условие только отвечает на вопрос: запись передана по константной
// ссылке, и ни менять её, ни печатать условие не должно. Иначе подсчёт
// напечатал бы то, что должен был только посчитать.
using Predicate = std::function<bool(const LogRecord& record)>;

// Отбор: указатели на подходящие записи журнала, в порядке журнала.
// Указатели годны, пока журнал не изменился и не заменён.
std::vector<const LogRecord*> select(const LogBook& book, const Predicate& condition);

// Сколько записей подходит, — без построения списка.
long count_matching(const LogBook& book, const Predicate& condition);

// Условия. Каждая функция возвращает лямбду, захватившую параметры.
Predicate any_record();
Predicate by_levels(LevelFilter levels);
Predicate by_module(std::string module);   // без учёта регистра
Predicate by_text(std::string word);       // подстрока без учёта регистра
Predicate between(Timestamp from, Timestamp to);   // обе границы включительно

// Оба условия сразу.
Predicate both(Predicate first, Predicate second);
