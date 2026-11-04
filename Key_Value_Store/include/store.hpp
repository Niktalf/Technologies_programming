#pragma once

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "journal_file.hpp"
#include "kvstore.hpp"
#include "record.hpp"

// Хранилище ключ-значение с журналом на диске. Внутренний класс
// библиотеки: клиенты видят его только через KvStore из include/kvstore.hpp.
// Консоль — инструмент самой библиотеки, ей нужны подробности журнала
// (команды stats и record), поэтому она работает с этим классом напрямую.
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
// чем таблица изменится. У закрытого хранилища всё по нулям.
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
class Store {
public:
    // Закрытое хранилище: журнала нет, таблица пуста.
    Store() = default;

    // Открывает журнал path. Файла может не быть — тогда создаётся пустая
    // база. Повреждённый хвост журнала отрезается. Если журнал не открылся —
    // KvError, и объекта нет.
    explicit Store(const std::string& path);

    ~Store();

    Store(const Store&) = delete;
    Store& operator=(const Store&) = delete;
    Store(Store&&) = default;
    Store& operator=(Store&&) = default;

    bool is_open() const { return journal_.is_open(); }
    const std::string& path() const { return journal_.path(); }

    // Операции. Каждая сначала пишет в журнал, потом меняет память.
    // existed — был ли ключ до операции.
    KvStatus put(const std::string& key, const std::string& value, bool& existed);
    KvStatus get(const std::string& key, std::string& value) const;
    KvStatus remove(const std::string& key);
    KvStatus apply(const Record& record, bool& existed);

    std::size_t size() const { return data_.size(); }

    // Сколько записей в журнале и сколько из них уже не нужны.
    long journal_records() const { return records_; }
    long journal_garbage() const { return garbage_; }

    // Сколько записей отброшено при открытии как повреждённые.
    long recovered_broken() const { return broken_; }

    // Все пары в порядке таблицы, то есть в произвольном.
    std::vector<std::pair<std::string, std::string>> pairs() const;

    // На практике 10 это было открытое статическое поле; теперь флаг закрыт.
    static void set_trace(bool on) { trace_ = on; }

private:
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
