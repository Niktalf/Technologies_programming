#pragma once

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "engine.hpp"
#include "journal_file.hpp"
#include "kvstore.hpp"
#include "record.hpp"

// Журнальный движок: хеш-таблица в памяти и журнал на диске.
//
// Практика 12: это наследник Engine. До практики 11 класс назывался Store
// и был единственным устройством хранилища; теперь он — один из двух
// движков, и всё, кроме опытов demo и stress, работает с ним через Engine.
//
// Инвариант — то, что связывает таблицу в памяти и журнал на диске:
//   * data_ — ровно то, что получится, если применить по порядку все
//     целые записи журнала к пустой таблице;
//   * records_ — число целых записей в журнале, и
//     records_ == data_.size() + garbage_: каждая запись либо держит живую
//     пару, либо уже не нужна;
//   * broken_ — сколько записей отброшено при открытии, 0 или 1.
// Держится он порядком действий в apply: сначала запись в журнал, потом
// таблица. Если запись в журнал не удалась, исключение вылетит раньше,
// чем таблица изменится. У объекта, из которого журнал забрали
// перемещением, всё по нулям.
//
// Практика 11: хранилище больше не работает с файлом. Чтение журнала,
// обрезка хвоста, байты записей — внутри JournalFile; здесь — таблица
// и логика операций.
//
// Практика 10: хранилище открывается конструктором и закрывается
// деструктором. Конструктор открывает журнал, перечитывает его и собирает
// таблицу; каждая операция дописывается в журнал; деструктор поля journal_
// сбрасывает буфер и закрывает файл. Методов open и close больше нет.
//
// Правило пяти. Деструктор объявлен (он печатает сообщение в опытах),
// и из-за этого компилятор сам перемещение не создаст. Поэтому все пять
// особых методов объявлены явно: копирование запрещено, перемещение —
// созданное компилятором по умолчанию: оно перемещает поле за полем,
// а JournalFile умеет перемещаться сам.
class JournalEngine : public Engine {
public:
    // Открывает журнал path. Файла может не быть — тогда создаётся пустая
    // база. Повреждённый хвост журнала отрезается. Если журнал не открылся —
    // KvError, и объекта нет.
    explicit JournalEngine(const std::string& path);

    ~JournalEngine() override;

    JournalEngine(const JournalEngine&) = delete;
    JournalEngine& operator=(const JournalEngine&) = delete;
    JournalEngine(JournalEngine&&) = default;
    JournalEngine& operator=(JournalEngine&&) = default;

    bool is_open() const { return journal_.is_open(); }
    const std::string& path() const { return journal_.path(); }

    // Операции Engine. Каждая сначала пишет в журнал, потом меняет память.
    KvStatus put(const std::string& key, const std::string& value, bool& existed) override;
    KvStatus get(const std::string& key, std::string& value) const override;
    KvStatus remove(const std::string& key) override;
    long for_each(const Visitor& visit) const override;
    std::size_t size() const override { return data_.size(); }
    bool repaired() const override { return broken_ > 0; }

    // Записи журнала, ненужные среди них, отброшенные при открытии и доля
    // мусора в журнале.
    std::string describe() const override;

    // Практика 14: мусор журнала. Проходит по всем записям файла и решает,
    // какие из них актуальны: запись актуальна, если она последняя для
    // своего ключа и это put. Остальное — заменённые значения, удалённые
    // пары и надгробия — мусор. live — байт в актуальных записях,
    // total — во всех.
    void garbage(long long& live, long long& total) const;

    // Только у журнального движка: для опытов и для проверки инварианта.
    long journal_records() const { return records_; }
    long journal_garbage() const { return garbage_; }

    // На практике 10 это было открытое статическое поле; теперь флаг закрыт.
    static void set_trace(bool on) { trace_ = on; }

private:
    KvStatus apply(const Record& record, bool& existed);
    void apply_in_memory(const Record& record);

    // Поля создаются в порядке объявления и разрушаются в обратном:
    // журнал создаётся первым и закрывается последним.
    JournalFile journal_;
    std::unordered_map<std::string, std::string> data_;
    long records_ = 0;   // записей в журнале
    long garbage_ = 0;   // из них уже не нужных
    long broken_ = 0;    // записей, отброшенных при открытии как повреждённые

    static bool trace_;
};
