#include "journal_engine.hpp"

#include <algorithm>
#include <cstdio>
#include <iostream>
#include <numeric>
#include <unordered_map>

bool JournalEngine::trace_ = false;

// Список инициализации: journal_ создаётся до входа в тело конструктора.
// Если его конструктор возбудит исключение, тело JournalEngine
// не выполнится, и объекта не будет вовсе.
JournalEngine::JournalEngine(const std::string& path)
    : journal_(path) {
    // Журнал отдаёт все целые записи по порядку и сам отрезает повреждённый
    // хвост. Если он возбудит исключение, объекта JournalEngine не будет, но уже
    // созданное поле journal_ разрушится своим деструктором — файл всё
    // равно закроется.
    std::vector<Record> records;
    broken_ = journal_.recover(records);
    // Таблица собирается применением записей по порядку — это и есть
    // первая половина инварианта.
    for (const Record& record : records) {
        apply_in_memory(record);
        ++records_;
    }
    if (trace_) {
        std::cout << "  [JournalEngine] opened " << path << ", keys " << data_.size() << '\n';
    }
}

JournalEngine::~JournalEngine() {
    // Закрывать журнал здесь не нужно: после тела этого деструктора
    // автоматически вызовется деструктор поля journal_.
    if (trace_ && journal_.is_open()) {
        std::cout << "  [JournalEngine] closing " << journal_.path() << '\n';
    }
}

// Применяет запись к таблице. Побеждает последняя запись о ключе,
// прежние становятся мусором.
void JournalEngine::apply_in_memory(const Record& record) {
    if (record.operation() == Record::Operation::remove) {
        if (data_.erase(record.key()) > 0) {
            ++garbage_;   // удалённая пара стала мусором
        }
        ++garbage_;       // и сама запись об удалении тоже
        return;
    }
    if (!data_.insert_or_assign(record.key(), record.value()).second) {
        ++garbage_;
    }
}

KvStatus JournalEngine::apply(const Record& record, bool& existed) {
    existed = data_.count(record.key()) > 0;
    if (!journal_.is_open() || !record.valid()) {
        return KvStatus::bad_argument;
    }
    // Удаление отсутствующего ключа в журнал не пишется: менять нечего.
    if (record.operation() == Record::Operation::remove && !existed) {
        return KvStatus::not_found;
    }
    // Сначала журнал, потом память. Если запись в журнал не удалась,
    // append возбудит исключение, и таблица останется прежней.
    journal_.append(record);
    ++records_;
    apply_in_memory(record);
    return KvStatus::ok;
}

KvStatus JournalEngine::put(const std::string& key, const std::string& value, bool& existed) {
    return apply(Record::put(key, value), existed);
}

KvStatus JournalEngine::remove(const std::string& key) {
    bool existed = false;
    return apply(Record::remove(key), existed);
}

KvStatus JournalEngine::get(const std::string& key, std::string& value) const {
    auto found = data_.find(key);
    if (found == data_.end()) {
        return KvStatus::not_found;
    }
    value = found->second;
    return KvStatus::ok;
}

long JournalEngine::for_each(const Visitor& visit) const {
    // Цикл идёт, пока visit отвечает true. Ответ false прекращает обход:
    // остальные пары не перебираются вовсе.
    long visited = 0;
    bool going = true;
    auto pair = data_.begin();
    while (going && pair != data_.end()) {
        ++visited;
        going = visit(pair->first, pair->second);
        ++pair;
    }
    return visited;
}

void JournalEngine::garbage(long long& live, long long& total) const {
    std::vector<Record> records = journal_.records();
    // map: размер каждой записи в байтах — ровно столько она занимает в файле.
    std::vector<long long> sizes(records.size());
    std::transform(records.begin(), records.end(), sizes.begin(), [](const Record& record) {
        return static_cast<long long>(record.encode().size());
    });
    total = std::accumulate(sizes.begin(), sizes.end(), 0LL);

    // Номер последней записи каждого ключа. Группировка словарём: проход
    // по номерам записей, и более поздняя запись затирает более раннюю.
    std::vector<std::size_t> numbers(records.size());
    std::iota(numbers.begin(), numbers.end(), std::size_t{ 0 });
    std::unordered_map<std::string, std::size_t> last;
    std::for_each(numbers.begin(), numbers.end(),
                  [&records, &last](std::size_t i) { last[records[i].key()] = i; });

    // filter + reduce: складываем размеры только актуальных записей.
    // Надгробие — последняя запись удалённого ключа, но актуальным оно
    // не считается: пары больше нет, а место надгробие занимает.
    live = std::accumulate(numbers.begin(), numbers.end(), 0LL,
                           [&](long long sum, std::size_t i) {
                               bool current = last[records[i].key()] == i
                                              && records[i].operation() == Record::Operation::put;
                               return current ? sum + sizes[i] : sum;
                           });
}

std::string JournalEngine::describe() const {
    long long live = 0;
    long long total = 0;
    garbage(live, total);
    // Пустой журнал: мусора нет, а не «ноль делить на ноль».
    double share = total > 0 ? 1.0 - static_cast<double>(live) / static_cast<double>(total) : 0.0;
    char percent[32];
    std::snprintf(percent, sizeof percent, "%.1f", share * 100.0);
    return "Journal records: " + std::to_string(records_) + ", obsolete among them: "
           + std::to_string(garbage_) + "\nDiscarded as damaged on open: "
           + std::to_string(broken_) + "\nGarbage in the journal: " + percent + "% ("
           + std::to_string(total - live) + " of " + std::to_string(total) + " bytes)\n";
}
