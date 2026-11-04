#pragma once

#include <cstddef>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

// Публичный заголовок библиотеки ключ-значение. Всё, что видит клиент.
//
// Здесь нет ни хеш-таблицы, ни формата записей, ни смещений в файле, ни
// даже слова «журнал»: как хранилище устроено внутри, клиенту знать
// не нужно. Внутренние заголовки лежат в src/ и клиентам не видны вовсе —
// CMake даёт им только каталог include/. Попробуй телефонный справочник
// включить store.hpp — он просто не соберётся.

// Результат операции.
enum class KvStatus { ok, bad_argument, not_found, cannot_open, io_error };

// Текст для пользователя.
const char* status_text(KvStatus status);

// Ошибка открытия базы или записи на диск. Кроме текста хранит код.
class KvError : public std::runtime_error {
public:
    KvError(KvStatus status, const std::string& path);

    KvStatus status() const { return status_; }

private:
    KvStatus status_;
};

// Движок хранения — внутренний абстрактный класс. Здесь только объявление:
// указатель на неполный тип можно хранить, не зная, что внутри. Тот же
// приём, что неполный тип KvStore в публичном заголовке версии на C
// (практика 8).
class Engine;

// Хранилище ключ-значение, сохраняющее данные между запусками.
//
// Операции — и почему каждая нужна клиенту:
//   * KvStore(path) — открыть базу: без этого не с чем работать.
//     Имя «:memory:» — база только в памяти, на диск ничего не пишется
//     (практика 12). Другое имя — файл на диске;
//   * put — сохранить или заменить значение: ради этого всё и затевалось;
//   * get — прочитать значение по ключу;
//   * remove — удалить пару: телефон человека, которого больше нет в книге;
//   * size — сколько пар, не копируя их: «в книге 120 записей»;
//   * pairs — все пары: показать справочник целиком;
//   * repaired — была ли база повреждена при прошлом сбое и что-то из неё
//     потеряно: клиент обязан честно сказать это пользователю.
// Чего нет и почему. Нет счётчиков записей на диске — это подробность
// устройства. Нет закрытия — его делает деструктор. Нет копирования — два
// объекта писали бы в одну базу вперемешку.
//
// Инвариант: объект открыт — владеет открытой базой. Пустым его делает
// только перемещение; у пустого все операции отвечают bad_argument.
class KvStore {
public:
    // Имя базы, которая живёт только в памяти.
    static constexpr const char* IN_MEMORY = ":memory:";

    // Ключ — от 1 до KEY_MAX байт, значение — до VALUE_MAX байт.
    static constexpr std::size_t KEY_MAX = 255;
    static constexpr std::size_t VALUE_MAX = 4096;

    // Открывает базу path; если файла нет, создаёт пустую. path == IN_MEMORY —
    // база в памяти. Не открылась — KvError, и объекта нет.
    explicit KvStore(const std::string& path);

    // Деструктор объявлен здесь, а определён в kvstore.cpp: только там
    // Engine — полный тип, и unique_ptr знает, как его удалить.
    ~KvStore();

    KvStore(const KvStore&) = delete;
    KvStore& operator=(const KvStore&) = delete;
    KvStore(KvStore&& other) noexcept;
    KvStore& operator=(KvStore&& other) noexcept;

    // Неверная длина ключа или значения — bad_argument. Сбой записи
    // на диск — KvError.
    KvStatus put(const std::string& key, const std::string& value);
    KvStatus get(const std::string& key, std::string& value) const;
    KvStatus remove(const std::string& key);

    std::size_t size() const;
    std::vector<std::pair<std::string, std::string>> pairs() const;
    bool repaired() const;

private:
    // Движок живёт в куче, а здесь — только указатель на его базовый класс.
    // Какой движок под ним — журнальный или в памяти, — KvStore не знает:
    // все его методы — виртуальные вызовы. Поэтому заголовку не нужны
    // ни движки, ни unordered_map, ни формат записей, ни файл журнала.
    std::unique_ptr<Engine> engine_;
};
