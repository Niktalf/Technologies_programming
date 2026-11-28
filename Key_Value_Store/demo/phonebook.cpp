// Демонстрационная программа: телефонный справочник поверх библиотеки.
//
// Практика 11: справочник включает только публичный заголовок kvstore.hpp
// и знает только класс KvStore. Ни хеш-таблицы, ни формата записей, ни
// журнала он не видит: CMake даёт ему один каталог include/, и внутренние
// заголовки из src/ ему просто недоступны. Если завтра внутренности
// библиотеки переписать целиком, эту программу менять не придётся.
//
// На практике 10 справочник печатал число записей в журнале и число
// ненужных — для этого ему нужен был сам Store. Такие подробности
// пользователю справочника ни к чему, и из публичного интерфейса их нет.

#include <iomanip>
#include <iostream>
#include <string>

#include "kvstore.hpp"

namespace {

void print_usage(const char* program) {
    std::cout << "Phone book.\n"
              << "  " << program << " <file> add <name> <phone>\n"
              << "  " << program << " <file> find <name>\n"
              << "  " << program << " <file> remove <name>\n"
              << "  " << program << " <file> list\n"
              << "  " << program << " <file> prefix <start of the name>\n";
}

int add(KvStore& book, const std::string& name, const std::string& phone) {
    KvStatus status = book.put(name, phone);
    if (status != KvStatus::ok) {
        std::cout << "Cannot save: " << status_text(status) << '\n';
        return 1;
    }
    std::cout << "Saved.\n";
    return 0;
}

int find(const KvStore& book, const std::string& name) {
    std::string phone;
    if (book.get(name, phone) != KvStatus::ok) {
        std::cout << "No such name.\n";
        return 1;
    }
    std::cout << name << ": " << phone << '\n';
    return 0;
}

int remove_entry(KvStore& book, const std::string& name) {
    KvStatus status = book.remove(name);
    if (status != KvStatus::ok) {
        std::cout << "Cannot delete: " << status_text(status) << '\n';
        return 1;
    }
    std::cout << "Deleted.\n";
    return 0;
}

void print_entry(const std::string& name, const std::string& phone) {
    std::cout << "  " << std::left << std::setw(20) << name << std::right << ' ' << phone << '\n';
}

int list(const KvStore& book) {
    // Практика 13: справочник обходит записи, передавая хранилищу функцию.
    // Ни вектора пар, ни таблицы он не видит.
    long shown = book.for_each([](const std::string& name, const std::string& phone) {
        print_entry(name, phone);
        return true;
    });
    if (shown == 0) {
        std::cout << "The phone book is empty.\n";
    } else {
        std::cout << "Entries total: " << shown << '\n';
    }
    return 0;
}

// Все имена, начинающиеся с start. Через одну открытую операцию — обход:
// ни доступа к таблице, ни отдельной операции «поиск по префиксу» в
// библиотеке для этого не понадобилось.
int find_prefix(const KvStore& book, const std::string& start) {
    long found = 0;
    book.for_each([&start, &found](const std::string& name, const std::string& phone) {
        if (name.compare(0, start.size(), start) == 0) {
            print_entry(name, phone);
            ++found;
        }
        return true;
    });
    std::cout << "Names starting with \"" << start << "\": " << found << '\n';
    return 0;
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc < 3) {
        print_usage(argv[0]);
        return 1;
    }
    try {
        KvStore book(argv[1]);
        if (book.repaired()) {
            std::cout << "Warning: the end of the file was damaged and discarded.\n";
        }
        const std::string action = argv[2];
        if (action == "add" && argc >= 5) {
            return add(book, argv[3], argv[4]);
        }
        if (action == "find" && argc >= 4) {
            return find(book, argv[3]);
        }
        if (action == "remove" && argc >= 4) {
            return remove_entry(book, argv[3]);
        }
        if (action == "list") {
            return list(book);
        }
        if (action == "prefix" && argc >= 4) {
            return find_prefix(book, argv[3]);
        }
        print_usage(argv[0]);
        return 1;
    } catch (const KvError& error) {
        // Сбой записи тоже приходит сюда: журнал закроет деструктор book.
        std::cerr << "Cannot open the phone book: " << error.what() << '\n';
        return 1;
    }   // при любом return журнал закрывает деструктор book
}
