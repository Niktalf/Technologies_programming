#include <iostream>
#include <string>
#include <vector>

#include "console.hpp"

// Практика 10: хранилище открывается конструктором и закрывается
// деструктором; файлом журнала владеет JournalFile. Всё, что умела
// программа на практике 9, осталось; добавились команды demo и stress.
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
    // Если журнал не открылся, конструктор Console не завершится:
    // исключение придёт сюда, и закрывать будет нечего. Это ошибка запуска —
    // в поток ошибок и код 1.
    try {
        Console console(first, "store2.db");
        if (argc > shift) {
            // Слова команды — аргументы программы. Вектор строк строится сразу
            // из диапазона указателей argv + shift … argv + argc.
            std::vector<std::string> words(argv + shift, argv + argc);
            return console.execute(words) == Console::Result::error ? 1 : 0;
        }
        console.run_interactive();
    } catch (const KvError& error) {
        std::cerr << "Cannot open the stores: " << error.what() << '\n';
        return 1;
    }
    return 0;   // хранилища закрыты деструктором console при выходе из блока try
}
