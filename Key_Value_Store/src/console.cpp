#include "console.hpp"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

#include "parser.hpp"
#include "sorted_pairs.hpp"

namespace {

// Целое число из слова: всё слово — число. false, если это не так.
bool parse_long(const std::string& word, long& out) {
    std::istringstream in(word);
    long value = 0;
    if (!(in >> value) || !in.eof()) {
        return false;
    }
    out = value;
    return true;
}

// Пустая строка или комментарий: после пробелов ничего нет или стоит решётка.
bool is_blank_or_comment(const std::string& line) {
    std::size_t first = line.find_first_not_of(" \t\r");
    return first == std::string::npos || line[first] == '#';
}

// Байты шестнадцатеричными числами через пробел.
void print_bytes(const std::vector<std::uint8_t>& bytes) {
    std::ios::fmtflags flags = std::cout.flags();
    char fill = std::cout.fill();
    std::cout << "Bytes:" << std::hex << std::uppercase << std::setfill('0');
    for (std::uint8_t byte : bytes) {
        std::cout << ' ' << std::setw(2) << static_cast<int>(byte);
    }
    std::cout.flags(flags);
    std::cout.fill(fill);
    std::cout << '\n';
}

} // namespace

KvStatus Console::open(const std::string& first, const std::string& second) {
    return session_.open(first, second);
}

void Console::close() {
    session_.close();
}

void Console::print_help() {
    std::cout << "Available commands:\n"
              << "  put <key> <value>    - store a pair\n"
              << "      a value with spaces goes in quotes: put name \"two words\"\n"
              << "      a quote inside a value is written as \\\"\n"
              << "  get <key>            - get the value\n"
              << "  del <key>            - delete the pair\n"
              << "  list                 - list all pairs\n"
              << "  sorted               - all pairs in ascending key order\n"
              << "  range <from> <to>    - pairs with keys in the range, bounds included\n"
              << "  stats                - keys and journal records\n"
              << "  use <1|2>            - switch to the other store\n"
              << "  which                - which store is selected and where its journal is\n"
              << "  copy <key>           - copy the pair to the other store\n"
              << "  open <file>          - open another journal in place of the current one\n"
              << "  close                - close the current store, the journal stays on disk\n"
              << "  script <file> [stop] - run commands from a file\n"
              << "  genscript <file> <n> - write a script of n put commands\n"
              << "  record <key> <val>   - build a journal record and check it\n"
              << "  help                 - this list\n"
              << "  quit                 - exit\n";
}

// Печатает ошибку хранилища. «Ключ не найден» — ответ, а не ошибка:
// скрипт из-за него не прерывается.
Console::Result Console::report(KvStatus status) const {
    std::cout << "Error: " << Store::status_text(status) << '\n';
    return status == KvStatus::not_found ? Result::ok : Result::error;
}

Console::Result Console::command_put(const std::vector<std::string>& words) {
    if (words.size() < 3) {
        std::cout << "put needs a key and a value\n";
        return Result::error;
    }
    // Запись создаётся и тут же применяется к хранилищу. Временный объект
    // Record живёт до конца этой строки.
    bool existed = false;
    KvStatus status = Record::put(words[1], words[2]).apply_to(session_.current(), existed);
    if (status != KvStatus::ok) {
        return report(status);
    }
    std::cout << (existed ? "updated\n" : "added\n");
    return Result::ok;
}

Console::Result Console::command_get(const std::vector<std::string>& words) {
    if (words.size() < 2) {
        std::cout << "get needs a key\n";
        return Result::error;
    }
    std::string value;
    KvStatus status = session_.current().get(words[1], value);
    if (status != KvStatus::ok) {
        return report(status);
    }
    std::cout << value << '\n';
    return Result::ok;
}

Console::Result Console::command_del(const std::vector<std::string>& words) {
    if (words.size() < 2) {
        std::cout << "del needs a key\n";
        return Result::error;
    }
    KvStatus status = session_.current().remove(words[1]);
    if (status != KvStatus::ok) {
        return report(status);
    }
    std::cout << "deleted\n";
    return Result::ok;
}

Console::Result Console::command_list() {
    if (session_.current().size() == 0) {
        std::cout << "The store is empty.\n";
        return Result::ok;
    }
    // Порядок — порядок таблицы, то есть произвольный. Упорядоченный
    // вывод — команда sorted.
    for (const auto& pair : session_.current().pairs()) {
        std::cout << pair.first << " = " << pair.second << '\n';
    }
    return Result::ok;
}

Console::Result Console::command_sorted() {
    SortedPairs sorted;
    int depth = sorted.build(session_.current());
    int found = sorted.print_all();
    std::cout << "Pairs total: " << found << ", sort recursion depth: " << depth << '\n';
    return Result::ok;
}

Console::Result Console::command_range(const std::vector<std::string>& words) {
    if (words.size() < 3) {
        std::cout << "range needs two bounds\n";
        return Result::error;
    }
    if (words[1].compare(words[2]) > 0) {
        std::cout << "The lower bound is greater than the upper one: the range is empty.\n";
        return Result::ok;
    }
    SortedPairs sorted;
    int depth = sorted.build(session_.current());
    int found = sorted.print_range(words[1], words[2]);
    std::cout << "Pairs found: " << found << ", sort recursion depth: " << depth << '\n';
    return Result::ok;
}

Console::Result Console::command_stats() {
    const Store& store = session_.current();
    std::cout << "Keys: " << store.size() << '\n';
    std::cout << "Journal records: " << store.journal_records()
              << ", obsolete among them: " << store.journal_garbage() << '\n';
    std::cout << "Discarded as damaged on open: " << store.recovered_broken() << '\n';
    return Result::ok;
}

Console::Result Console::command_copy(const std::vector<std::string>& words) {
    if (words.size() < 2) {
        std::cout << "copy needs a key\n";
        return Result::error;
    }
    if (!session_.other().is_open()) {
        std::cout << "The other store is closed\n";
        return Result::error;
    }
    std::string value;
    bool existed = false;
    KvStatus status = session_.current().get(words[1], value);
    if (status == KvStatus::ok) {
        status = session_.other().put(words[1], value, existed);
    }
    if (status != KvStatus::ok) {
        return report(status);
    }
    std::cout << "Copied to the other store\n";
    return Result::ok;
}

Console::Result Console::command_close() {
    std::size_t count = session_.current().size();
    session_.current().close();
    std::cout << "Store " << session_.number() << " closed, pairs freed: " << count
              << ". Journal " << session_.path(session_.number() - 1) << " stays on disk.\n";
    return Result::ok;
}

Console::Result Console::command_use(const std::vector<std::string>& words) {
    if (words.size() < 2) {
        std::cout << "use needs a store number\n";
        return Result::error;
    }
    long number = 0;
    if (!parse_long(words[1], number) || !session_.select(static_cast<int>(number))) {
        std::cout << "The store number is from 1 to " << Session::STORES << '\n';
        return Result::error;
    }
    std::cout << "Selected store " << session_.number() << ", journal "
              << session_.path(session_.number() - 1) << ", "
              << (session_.current().is_open() ? "open" : "closed") << '\n';
    return Result::ok;
}

Console::Result Console::command_which() {
    std::cout << "Current store: " << session_.number() << ", journal "
              << session_.path(session_.number() - 1) << ", ";
    if (session_.current().is_open()) {
        std::cout << "keys: " << session_.current().size() << '\n';
    } else {
        std::cout << "closed\n";
    }
    return Result::ok;
}

Console::Result Console::command_open(const std::vector<std::string>& words) {
    if (words.size() < 2) {
        std::cout << "open needs a journal file name\n";
        return Result::error;
    }
    KvStatus status = session_.open_current(words[1]);
    if (status != KvStatus::ok) {
        return report(status);
    }
    const Store& store = session_.current();
    std::cout << "Journal opened " << words[1] << ": keys " << store.size()
              << ", damaged records discarded " << store.recovered_broken() << '\n';
    return Result::ok;
}

// Выполняет команды из файла. О любой ошибке сообщает с номером строки.
// Скрипт прерывают команда выхода и, если просили, первая ошибка.
Console::Result Console::command_script(const std::vector<std::string>& words) {
    if (words.size() < 2) {
        std::cout << "script needs a file name\n";
        return Result::error;
    }
    std::ifstream file(words[1]);
    if (!file) {
        std::cout << "Cannot open the script: " << words[1] << '\n';
        return Result::ok;   // команда script выполнилась, хоть и без толку — как в версии на C
    }
    bool stop_on_error = words.size() >= 3 && words[2] == "stop";
    long lines = 0;
    long executed = 0;
    long failed = 0;
    bool running = true;
    std::string line;
    while (running && std::getline(file, line)) {
        ++lines;
        Result result = Result::ok;
        std::vector<std::string> command;
        if (is_blank_or_comment(line)) {
            result = Result::ok;
        } else if (!Parser::split(line, command)) {
            std::cout << "Line " << lines << ": unclosed quote\n";
            result = Result::error;
        } else if (!command.empty()) {
            ++executed;
            result = execute(command);
            if (result == Result::quit) {
                std::cout << "Line " << lines << ": exit command, script stopped\n";
            } else if (result == Result::error) {
                std::cout << "Line " << lines << ": command failed\n";
            }
        }
        if (result == Result::error) {
            ++failed;
            if (stop_on_error) {
                std::cout << "Execution stopped.\n";
                running = false;
            }
        } else if (result == Result::quit) {
            running = false;
        }
    }
    std::cout << "Lines in the file: " << lines << ", commands run: " << executed
              << ", errors: " << failed << '\n';
    return Result::ok;
}

// Пишет скрипт из n команд put. Руками файл на тысячу строк не набрать,
// а проверять разбор скриптов на нём нужно.
Console::Result Console::command_genscript(const std::vector<std::string>& words) {
    if (words.size() < 3) {
        std::cout << "genscript needs a file name and a number of pairs\n";
        return Result::error;
    }
    long n = 0;
    if (!parse_long(words[2], n) || n <= 0 || n > 10000000) {
        std::cout << "The number of pairs must be from 1 to 10000000\n";
        return Result::error;
    }
    std::ofstream file(words[1]);
    if (!file) {
        std::cout << "Cannot create the file: " << words[1] << '\n';
        return Result::error;
    }
    file << "# script of " << n << " pairs, made by genscript\n";
    for (long i = 1; i <= n; ++i) {
        file << "put key" << std::setw(6) << std::setfill('0') << i << " \"value number " << i
             << "\"\n";
    }
    // Ошибка записи (например, кончилось место на диске) видна только
    // при закрытии файла, поэтому файл закрывается явно и проверяется.
    file.close();
    if (!file) {
        std::cout << "Error writing the file: " << words[1] << '\n';
        return Result::error;
    }
    std::cout << "Script written " << words[1] << ": " << n << " commands\n";
    return Result::ok;
}

// Практика 2: сборка записи журнала, проверка и обнаружение порчи.
// Такими записями хранилище пишет журнал на диск.
Console::Result Console::command_record(const std::vector<std::string>& words) {
    if (words.size() < 3) {
        std::cout << "record needs a key and a value\n";
        return Result::error;
    }
    Record record = Record::put(words[1], words[2]);
    if (!record.valid()) {
        std::cout << "Build error: key or value too long\n";
        return Result::error;
    }
    std::vector<std::uint8_t> bytes = record.encode();
    std::cout << "Record size: " << bytes.size() << " bytes (header " << Record::HEADER_SIZE
              << " + key " << words[1].size() << " + value " << words[2].size() << ")\n";
    print_bytes(bytes);
    std::cout << "Parse: " << (Record::check(bytes) ? "success" : "checksum mismatch") << '\n';
    Record::print_flags(bytes[0]);
    std::ios::fmtflags flags = std::cout.flags();
    std::cout << "Key length: " << words[1].size() << ", value length: " << words[2].size()
              << ", checksum: 0x" << std::hex << std::uppercase << std::setw(2)
              << std::setfill('0') << static_cast<int>(bytes[Record::HEADER_SIZE - 1]) << '\n'
              << std::setfill(' ');
    std::cout.flags(flags);

    // Порча байта данных: длина сходится, порчу ловит контрольная сумма.
    // Повторное исключающее ИЛИ с той же маской возвращает байт обратно.
    bytes.back() ^= 0x01u;
    std::cout << "After corrupting the last byte: "
              << (Record::check(bytes) ? "success" : "checksum mismatch") << '\n';
    bytes.back() ^= 0x01u;

    // Порча длины ключа: её ловит проверка размера, которая стоит раньше.
    bytes[2] ^= 0x01u;
    std::cout << "After corrupting the key length: "
              << (Record::check(bytes) ? "success" : "checksum mismatch") << '\n';
    return Result::ok;
}

// Команды, которым нужно открытое хранилище.
Console::Result Console::execute_on_store(const std::vector<std::string>& words) {
    const std::string& name = words[0];
    if (name == "put") {
        return command_put(words);
    }
    if (name == "get") {
        return command_get(words);
    }
    if (name == "del") {
        return command_del(words);
    }
    if (name == "list") {
        return command_list();
    }
    if (name == "sorted") {
        return command_sorted();
    }
    if (name == "range") {
        return command_range(words);
    }
    if (name == "stats") {
        return command_stats();
    }
    if (name == "copy") {
        return command_copy(words);
    }
    if (name == "close") {
        return command_close();
    }
    std::cout << "Unknown command: " << name << '\n';
    std::cout << "Type help for the list of commands.\n";
    return Result::error;
}

Console::Result Console::execute(const std::vector<std::string>& words) {
    if (words.empty()) {
        return Result::ok;
    }
    // Сначала команды, которым открытое хранилище не нужно.
    const std::string& name = words[0];
    if (name == "quit" || name == "exit") {
        return Result::quit;
    }
    if (name == "help") {
        print_help();
        return Result::ok;
    }
    if (name == "use") {
        return command_use(words);
    }
    if (name == "which") {
        return command_which();
    }
    if (name == "open") {
        return command_open(words);
    }
    if (name == "script") {
        return command_script(words);
    }
    if (name == "genscript") {
        return command_genscript(words);
    }
    if (name == "record") {
        return command_record(words);
    }
    // Всё остальное работает с данными и требует открытого хранилища.
    if (!session_.current().is_open()) {
        std::cout << "Store " << session_.number() << " is closed. Open it: open <file>\n";
        return Result::error;
    }
    return execute_on_store(words);
}

void Console::run_interactive() {
    std::cout << "Key-value store. Journals: " << session_.path(0) << " and "
              << session_.path(1) << ".\n";
    std::cout << "Type help for the list of commands.\n";
    // Конец ввода и команда quit сводятся к одному флагу: у цикла одно
    // условие выхода. Пустая строка даёт ноль слов, и execute её пропускает.
    bool running = true;
    while (running) {
        std::cout << "> " << std::flush;
        std::string line;
        std::vector<std::string> words;
        if (!std::getline(std::cin, line)) {
            running = false;
        } else if (!Parser::split(line, words)) {
            std::cout << "Cannot parse the line: unclosed quote\n";
        } else {
            running = execute(words) != Result::quit;
        }
    }
    std::cout << "Done.\n";
}
