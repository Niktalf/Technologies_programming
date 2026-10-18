#pragma once

#include <cstdint>
#include <string>

// Уровни важности. enum class, в отличие от enum из C, не превращается
// в число молча: уровень нельзя случайно сложить с номером строки.
enum class Level { debug, info, warn, error, fatal };

constexpr int LEVEL_COUNT = 5;

// Слово из журнала в уровень, без учёта регистра — как в версии на C.
// Возвращает false, если слово не уровень.
bool level_from_word(const std::string& word, Level& out);

// Уровень в слово для вывода: DEBUG, INFO, WARN, ERROR, FATAL.
const char* level_name(Level level);

// Фильтр по уровням: набор битов, бит номер i — уровень номер i.
// На практике 2 это было число LevelMask и функции level_mask_set,
// level_mask_has, получавшие его параметром. Здесь число спрятано в класс.
class LevelFilter {
public:
    // Все уровни проходят или ни один.
    void allow_all();
    void allow_none();

    void allow(Level level);
    bool allows(Level level) const;
    bool empty() const { return mask_ == 0; }

    // Список уровней и сама маска шестнадцатеричным числом.
    void print() const;

private:
    static std::uint8_t bit(Level level) {
        return static_cast<std::uint8_t>(1u << static_cast<int>(level));
    }

    std::uint8_t mask_ = (1u << LEVEL_COUNT) - 1u;   // по умолчанию проходят все
};
