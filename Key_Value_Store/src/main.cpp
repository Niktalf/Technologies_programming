#include <iostream>
#include <string>
#include <vector>

#include "console.hpp"

// Практика 9: проект переписан на C++. Всё, что умело хранилище на практике 8,
// осталось, но устроено классами: запись, хранилище, два хранилища, разбор
// строки, упорядоченные пары и консоль.
//
//   ./kvstore                        — интерактивный режим
//   ./kvstore put name "Ivan Petrov" — одна команда и выход
//   ./kvstore -f book.db list        — первый журнал — другой файл
//
// Данные живут в журналах store1.db и store2.db: можно положить ключ одним
// запуском и прочитать другим.
int main(int argc, char* argv[]) {
    std::string first = "store1.db";
    int shift = 1;
    if (argc >= 2 && std::string(argv[1]) == "-f") {
        if (argc < 3) {
            std::cerr << "Usage: " << argv[0] << " [-f <journal>] [command]\n";
            return 1;
        }
        first = argv[2];
        shift = 3;
    }
    // Консоль весит немного: таблицы и строки лежат в куче, их выделяют
    // стандартные контейнеры.
    Console console;
    KvStatus status = console.open(first, "store2.db");
    if (status != KvStatus::ok) {
        std::cerr << "Cannot open the stores: " << Store::status_text(status) << '\n';
        return 1;
    }
    int result = 0;
    if (argc > shift) {
        // Слова команды — аргументы программы. Вектор строк строится сразу
        // из диапазона указателей argv + shift … argv + argc.
        std::vector<std::string> words(argv + shift, argv + argc);
        result = console.execute(words) == Console::Result::error ? 1 : 0;
    } else {
        console.run_interactive();
    }
    // Закрыть хранилища нужно явно: объект консоли сам этого не сделает.
    // На практике 10 эту строку заменит деструктор.
    console.close();
    return result;
}
