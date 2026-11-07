// Демонстрационная программа: телефонный справочник поверх класса Store.
//
// Она знает только открытые методы хранилища и ничего не знает ни про
// хеш-таблицу, ни про формат журнала. Если завтра внутренности Store
// переписать целиком, эту программу менять не придётся.

#include <iomanip>
#include <iostream>
#include <string>

#include "store.hpp"

namespace {

void print_usage(const char* program) {
    std::cout << "Phone book.\n"
              << "  " << program << " <file> add <name> <phone>\n"
              << "  " << program << " <file> find <name>\n"
              << "  " << program << " <file> remove <name>\n"
              << "  " << program << " <file> list\n";
}

int add(Store& book, const std::string& name, const std::string& phone) {
    bool existed = false;
    KvStatus status = book.put(name, phone, existed);
    if (status != KvStatus::ok) {
        std::cout << "Cannot save: " << Store::status_text(status) << '\n';
        return 1;
    }
    std::cout << "Saved.\n";
    return 0;
}

int find(const Store& book, const std::string& name) {
    std::string phone;
    if (book.get(name, phone) != KvStatus::ok) {
        std::cout << "No such name.\n";
        return 1;
    }
    std::cout << name << ": " << phone << '\n';
    return 0;
}

int remove_entry(Store& book, const std::string& name) {
    KvStatus status = book.remove(name);
    if (status != KvStatus::ok) {
        std::cout << "Cannot delete: " << Store::status_text(status) << '\n';
        return 1;
    }
    std::cout << "Deleted.\n";
    return 0;
}

int list(const Store& book) {
    int shown = 0;
    for (const auto& entry : book.pairs()) {
        std::cout << "  " << std::left << std::setw(20) << entry.first << std::right << ' '
                  << entry.second << '\n';
        ++shown;
    }
    if (shown == 0) {
        std::cout << "The phone book is empty.\n";
    } else {
        std::cout << "Entries total: " << shown << '\n';
    }
    std::cout << "Journal records: " << book.journal_records()
              << ", no longer needed among them: " << book.journal_garbage() << '\n';
    return 0;
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc < 3) {
        print_usage(argv[0]);
        return 1;
    }
    try {
        Store book(argv[1]);
        if (book.recovered_broken() > 0) {
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
        print_usage(argv[0]);
        return 1;
    } catch (const KvError& error) {
        // Сбой записи тоже приходит сюда: журнал закроет деструктор book.
        std::cerr << "Cannot open the phone book: " << error.what() << '\n';
        return 1;
    }   // при любом return журнал закрывает деструктор book
}
