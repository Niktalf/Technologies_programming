// Вторая демонстрационная программа: счётчик посещений страниц.
//
// Практика 11. Написана только по публичному заголовку kvstore.hpp — как её
// написал бы человек, который видит библиотеку впервые. Если для неё
// чего-то не хватило бы, это был бы признак неудачного интерфейса:
// пока клиент один, любой интерфейс кажется удобным.
//
// Не хватило одного: «значение или ноль, если ключа нет». Но это делается
// через get и ответ not_found, и отдельная операция не нужна. Счётчик
// хранится строкой: хранилище знает только строки, а числа — дело клиента.
//
//   ./visits <file> <page>   — засчитать посещение и показать счётчик
//   ./visits <file>          — все страницы и общее число посещений

#include <iostream>
#include <sstream>
#include <string>

#include "kvstore.hpp"

namespace {

// Число из строки хранилища. Испорченное значение считается нулём.
long to_number(const std::string& text) {
    std::istringstream in(text);
    long value = 0;
    if (!(in >> value) || !in.eof() || value < 0) {
        return 0;
    }
    return value;
}

int count_visit(KvStore& visits, const std::string& page) {
    std::string stored;
    long count = 0;
    KvStatus status = visits.get(page, stored);
    if (status == KvStatus::ok) {
        count = to_number(stored);
    } else if (status != KvStatus::not_found) {
        std::cout << "Cannot read the counter: " << status_text(status) << '\n';
        return 1;
    }
    ++count;
    status = visits.put(page, std::to_string(count));
    if (status != KvStatus::ok) {
        std::cout << "Cannot save the counter: " << status_text(status) << '\n';
        return 1;
    }
    std::cout << page << ": " << count << (count == 1 ? " visit\n" : " visits\n");
    return 0;
}

int list(const KvStore& visits) {
    long total = 0;
    for (const auto& entry : visits.pairs()) {
        long count = to_number(entry.second);
        std::cout << "  " << entry.first << ": " << count << '\n';
        total += count;
    }
    std::cout << "Pages: " << visits.size() << ", visits in total: " << total << '\n';
    return 0;
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc < 2 || argc > 3) {
        std::cout << "Usage: " << argv[0] << " <file> [page]\n";
        return 1;
    }
    try {
        KvStore visits(argv[1]);
        if (visits.repaired()) {
            std::cout << "Warning: the end of the file was damaged and discarded.\n";
        }
        if (argc == 3) {
            return count_visit(visits, argv[2]);
        }
        return list(visits);
    } catch (const KvError& error) {
        std::cerr << "Cannot open the counter: " << error.what() << '\n';
        return 1;
    }
}
