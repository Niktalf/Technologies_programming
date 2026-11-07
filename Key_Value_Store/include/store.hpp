#pragma once

#include <cstdio>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "record.hpp"

// Результат операции хранилища — те же коды, что KvStatus в версии на C,
// кроме нехватки памяти: её стандартные контейнеры сообщают иначе,
// и об этом речь пойдёт на практике 10.
enum class KvStatus { ok, bad_argument, not_found, cannot_open, io_error };

// Хранилище ключ-значение с журналом на диске.
//
// Хеш-таблица — стандартная std::unordered_map. Свою вы написали
// на практиках 3 и 7; здесь время занятия дороже.
//
// Каждая операция дописывается в журнал и сразу сбрасывается на диск,
// при открытии журнал перечитывается и таблица собирается заново —
// как в версии на C (практика 8).
//
// Открывает и закрывает хранилище пара методов open и close. Забыть close —
// значит оставить файл открытым: объект о себе не позаботится. Копировать
// хранилище тоже нельзя: копия получила бы тот же FILE *, и два close
// закрыли бы один файл дважды. Обе беды уберут конструктор и деструктор
// на практике 10.
class Store {
public:
    static const char* status_text(KvStatus status);

    // Открывает журнал path. Файла может не быть — тогда создаётся пустая
    // база. Повреждённый хвост журнала отрезается. Если хранилище было
    // открыто, сначала закрывается. При ошибке хранилище остаётся закрытым.
    KvStatus open(const std::string& path);

    // Закрывает журнал и очищает таблицу. Закрыть закрытое безопасно.
    void close();

    bool is_open() const { return journal_ != nullptr; }
    const std::string& path() const { return path_; }

    // Операции. Каждая сначала пишет в журнал, потом меняет память:
    // если программа умрёт между ними, при следующем открытии запись
    // прочитается из журнала. existed — был ли ключ до операции.
    KvStatus put(const std::string& key, const std::string& value, bool& existed);
    KvStatus get(const std::string& key, std::string& value) const;
    KvStatus remove(const std::string& key);
    KvStatus apply(const Record& record, bool& existed);

    std::size_t size() const { return data_.size(); }

    // Сколько записей в журнале и сколько из них уже не нужны: по этой доле
    // на практике 15 будет запускаться уплотнение.
    long journal_records() const { return records_; }
    long journal_garbage() const { return garbage_; }

    // Сколько записей отброшено при открытии как повреждённые.
    long recovered_broken() const { return broken_; }

    // Все пары в порядке таблицы, то есть в произвольном.
    std::vector<std::pair<std::string, std::string>> pairs() const;

private:
    KvStatus append(const Record& record);
    void apply_in_memory(const Record& record);
    KvStatus recover(long& good_end);
    KvStatus cut_tail(long good_end);

    std::unordered_map<std::string, std::string> data_;
    std::FILE* journal_ = nullptr;   // nullptr — хранилище закрыто
    std::string path_;
    long records_ = 0;   // записей в журнале
    long garbage_ = 0;   // из них уже не нужных
    long broken_ = 0;    // записей, отброшенных при открытии как повреждённые
};
