#include "record_filter.hpp"

#include <algorithm>
#include <cctype>
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

// Единственный цикл отбора записей во всём проекте. Все выборки — grep,
// module, between, head, tail, сохранённое условие — идут через него
// и отличаются только условием.
std::vector<const LogRecord*> select(const LogBook& book, const Predicate& condition) {
    std::vector<const LogRecord*> selected;
    for (const LogRecord& record : book) {
        if (condition(record)) {
            // Указатель, а не копия: на миллионе записей копии заняли бы
            // столько же памяти, сколько сам журнал.
            selected.push_back(&record);
        }
    }
    return selected;
}

long count_matching(const LogBook& book, const Predicate& condition) {
    // Подсчёт без списка: std::count_if сам проходит по журналу и спрашивает
    // условие. Ни одной записи не копируется, ни одного указателя не
    // запоминается.
    return static_cast<long>(std::count_if(book.begin(), book.end(), condition));
}

// Все условия ниже — лямбды с захватом по значению. Функция, которая
// строит условие, закончится раньше, чем условие применят, поэтому
// параметры копируются внутрь лямбды. [&module] сослался бы на параметр,
// которого к моменту вызова уже нет.

Predicate any_record() {
    return [](const LogRecord&) { return true; };   // захватывать нечего
}

Predicate by_levels(LevelFilter levels) {
    return [levels](const LogRecord& record) { return levels.allows(record.level()); };
}

Predicate by_module(std::string module) {
    return [module](const LogRecord& record) {
        return same_word_ignore_case(record.module(), module);
    };
}

Predicate by_text(std::string word) {
    return [word](const LogRecord& record) { return contains_ignore_case(record.text(), word); };
}

Predicate between(Timestamp from, Timestamp to) {
    return [from, to](const LogRecord& record) {
        return !record.time().is_before(from) && !to.is_before(record.time());
    };
}

Predicate both(Predicate first, Predicate second) {
    // Условие из двух условий: функция принимает функции и возвращает
    // функцию. Второе условие не спрашивается, если первое уже ответило «нет».
    return [first, second](const LogRecord& record) { return first(record) && second(record); };
}
