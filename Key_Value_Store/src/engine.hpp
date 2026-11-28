#pragma once

#include <cstddef>
#include <memory>
#include <string>

#include "kvstore.hpp"

// Практика 12: движок хранения — абстрактный класс.
//
// Движок умеет четыре вещи: положить, получить, удалить, обойти все пары
// (практика 13: обход — с функцией, которую передаёт клиент).
// Как он это делает — хеш-таблица с журналом на диске или просто таблица
// в памяти, — решает наследник. Всё, что работает с хранилищем (консоль,
// KvStore, а через него справочник и счётчик посещений), видит только
// этот класс и не знает, какой движок под ним.
//
//   Engine
//   ├── JournalEngine   таблица в памяти и журнал на диске (практики 2–11)
//   └── MemoryEngine    только таблица в памяти; после выхода всё пропадает
//
// Договор, который обязан выполнять каждый наследник, проверяет общий
// набор проверок run_conformance (conformance.hpp). Чего договор НЕ обещает:
// что данные переживут перезапуск. Это свойство журнального движка, а не
// движка вообще: движок в памяти честно его не даёт.
class Engine {
public:
    // Виртуальный деструктор: движки удаляются через std::unique_ptr<Engine>.
    // Без virtual delete вызвал бы только ~Engine, и деструктор журнального
    // движка не выполнился бы — журнал не закрылся бы, буфер не сбросился бы
    // на диск. Стандарт называет такое удаление неопределённым поведением.
    virtual ~Engine() = default;

    // Копировать движок через базу нельзя: копия получила бы только часть
    // Engine — срезка. Да и копия журнального движка писала бы в тот же файл.
    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;

    // existed — был ли ключ до операции. Ключ от 1 до KvStore::KEY_MAX байт,
    // значение до KvStore::VALUE_MAX байт, иначе bad_argument.
    virtual KvStatus put(const std::string& key, const std::string& value, bool& existed) = 0;
    virtual KvStatus get(const std::string& key, std::string& value) const = 0;
    virtual KvStatus remove(const std::string& key) = 0;

    // Практика 13: обход с функцией. visit вызывается для каждой пары, пока
    // не вернёт false; тогда обход прекращается — оставшиеся пары не
    // перебираются вовсе. Порядок пар не обещан. Возвращает, сколько раз
    // был вызван visit.
    //
    // На практике 12 здесь был pairs(), возвращавший копию всех пар
    // в векторе. Обход отдаёт те же данные, не копируя их и не показывая,
    // в чём они лежат.
    virtual long for_each(const Visitor& visit) const = 0;

    // Обход пар с ключами от from до to включительно. Выражен через
    // for_each, поэтому одинаков для всех движков и с ним согласован:
    // те же пары, отобранные по ключу. Остановка тоже работает.
    long for_each_in_range(const std::string& from, const std::string& to,
                           const Visitor& visit) const;
    virtual std::size_t size() const = 0;

    // Были ли данные повреждены при открытии и что-то из них отброшено.
    // Движку в памяти повреждать нечего — он всегда отвечает false.
    virtual bool repaired() const = 0;

    // Подробности устройства для команды stats — строками, каждый движок
    // о себе. Консоль печатает их, не зная, что в них.
    virtual std::string describe() const = 0;

protected:
    Engine() = default;
    // Перемещение — для наследников: журнальный движок перемещается
    // в опыте 4 команды demo.
    Engine(Engine&&) = default;
    Engine& operator=(Engine&&) = default;

    // Подходят ли длины ключа и значения. Проверка одна на все движки.
    static bool valid_pair(const std::string& key, const std::string& value);
};

// Выбор движка — в одном месте на всю программу. location равно
// KvStore::IN_MEMORY — движок в памяти, иначе — журнальный движок с файлом
// location. Журнал не открылся — KvError.
std::unique_ptr<Engine> make_engine(const std::string& location);
